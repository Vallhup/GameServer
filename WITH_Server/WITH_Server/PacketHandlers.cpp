#include "pch.h"
#include "PacketHandlers.h"

#include "CharacterDataService.h"
#include "CharacterSpawnService.h"
#include "DynamicTaskTypes.h"
#include "ECS/System/Phase2/ResolveAbilityStateSystem.h"
#include "ECS/System/Phase2/ResolveLocomotionStateSystem.h"
#include "ECS/System/Phase2/ResolvePlayerNetworkCompensationSystem.h"
#include "ECS/System/Phase8/PlayerDeathStatePolicy.h"
#include "ECS/System/Phase8/ResolveDeathAndDespawnSystem.h"
#include "ExecutionContextTypes.h"
#include "ExecutionCoreTypes.h"
#include "ExecutionOps.h"
#include "FrameworkRuntime.h"
#include "FrameworkLog.h"
#include "CombatStatisticsCommands.h"
#include "CombatStatisticsTaskTypeIds.h"
#include "GameDataCatalog.h"
#include "LoginAuth.h"
#include "RegisterAccount.h"
#include "TitleCommands.h"
#include "WorldDef.h"
#include "NetworkRuntime.h"
#include "NetworkTimingService.h"
#include "ODBCDatabaseBackend.h"
#include "PacketFactory.h"
#include "PacketHandlerContext.h"
#include "PacketType.h"
#include "PartyCommandQueue.h"
#include "SessionFlowCommands.h"
#include "SessionFlowController.h"
#include "ServerPacketStager.h"
#include "System.h"
#include "SystemMetaHelper.h"
#include "ServerSessionSystem.h"
#include "WorldRuntime.h"
#include "ECS/GameplayRuntimeComponents.h"
#include "ECS/System/GameplaySystemUtil.h"
#include "IWorldTransitionRequestSink.h"

#include "Protocol.pb.h"
#include "SendBuffer.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <string_view>

// CombatStatisticsTaskTypeIds.h 의 extern 선언에 대응하는 정의.
// RegisterServerPacketHandlers() 에서 등록 후 채워진다.
DynamicTaskTypeId g_incrementMonsterKillCountResultTaskTypeId{ InvalidDynamicTaskTypeId };
DynamicTaskTypeId g_incrementDeathByMonsterCountResultTaskTypeId{ InvalidDynamicTaskTypeId };
DynamicTaskTypeId g_unlockTitleResultTaskTypeId{ InvalidDynamicTaskTypeId };

namespace
{
    constexpr const char* kLogCategory = "PacketHandlers";
    constexpr uint32_t kWorldTransitionRejectReasonServerUnavailable = 1;
    constexpr uint32_t kWorldTransitionRejectReasonRequestRejected = 2;
    constexpr uint32_t kLoginFailReasonMalformedPacket = 1;
    constexpr uint32_t kLoginFailReasonDuplicateLogin = 2;
    constexpr uint32_t kLoginFailReasonEntryStartFailed = 3;
    constexpr uint32_t kLoginFailReasonAuthRejected = 4;
    constexpr uint32_t kLoginFailReasonDatabaseUnavailable = 5;
    constexpr uint32_t kLoginFailReasonDatabaseError = 6;
    constexpr uint32_t kLoginFailReasonTimeout = 7;
    constexpr uint32_t kTitleEquipReasonSuccess = 0;
    constexpr uint32_t kTitleEquipReasonInvalidTitle = 3;
    constexpr uint32_t kTitleEquipReasonDatabaseError = 4;

    DynamicTaskTypeId g_loginAuthResultTaskTypeId{ InvalidDynamicTaskTypeId };
    DynamicTaskTypeId g_registerAccountResultTaskTypeId{ InvalidDynamicTaskTypeId };
    DynamicTaskTypeId g_getAccountTitlesResultTaskTypeId{
        InvalidDynamicTaskTypeId };
    DynamicTaskTypeId g_setEquippedTitleResultTaskTypeId{
        InvalidDynamicTaskTypeId };

    const char* SessionFlowResultCodeName(SessionFlowResultCode code) noexcept;

    struct PlayerInputTarget
    {
        Entity entity{ Entity::Null() };
        PlayerControlIdentityComp* identity{ nullptr };
        PlayerNetworkTimingComp* networkTiming{ nullptr };
        ActorInputComp* input{ nullptr };
        WorldRuntime* runtime{ nullptr };
    };

    // payloadKey = SendBuffer* 로 부터 RAII 소유권 획득
    SendBufferPtr AcquirePayload(const NodeExecContext& ctx) noexcept
    {
        const auto* inst =
            ctx.frame->dynamicTaskFrameTable->FindByNodeId(ctx.nodeId);
        if (!inst || inst->payloadKey == 0)
            return nullptr;
        return SendBufferPtr(reinterpret_cast<SendBuffer*>(inst->payloadKey));
    }

    SessionId ResolveSessionId(const NodeExecContext& ctx) noexcept
    {
        const auto* inst =
            ctx.frame->dynamicTaskFrameTable->FindByNodeId(ctx.nodeId);
        return inst != nullptr ? static_cast<SessionId>(inst->sessionId) : 0;
    }

    uint64_t ResolveRequestFrameIndex(const NodeExecContext& ctx) noexcept
    {
        (void)ctx;
        return 0;
    }

    // sessionId → worldScopeId 역산 (worldIdByScope span 순회)
    // sessionId → WorldRuntime* (nullptr이면 미입장)
    WorldRuntime* ResolveWorldRuntime(
        const NodeExecContext& ctx,
        SessionId sessionId,
        const SessionFlowController& sessionFlow) noexcept
    {
        const WorldId worldId = sessionFlow.FindCurrentWorldId(sessionId);
        if (!worldId.IsValid())
        {
            FWLOG_WARN(kLogCategory,
                "ResolveWorldRuntime failed: session has no currentWorldId (sid=%u)",
                sessionId);
            return nullptr;
        }

        const WorldId ctxWorldId = ctx.TryGetWorldId();
        if (ctxWorldId != worldId)
        {
            FWLOG_WARN(kLogCategory,
                "ResolveWorldRuntime failed: ctx world mismatch (sid=%u, sessionWorldId=%u, ctxWorldId=%u)",
                sessionId,
                worldId.GetRaw(),
                ctxWorldId.GetRaw());
            return nullptr;
        }

        WorldRuntime* const runtime = ctx.TryGetRuntime();
        if (runtime == nullptr)
        {
            FWLOG_WARN(kLogCategory,
                "ResolveWorldRuntime failed: ctx.TryGetRuntime() == nullptr (sid=%u, worldId=%u)",
                sessionId,
                worldId.GetRaw());
        }
        return runtime;
    }

    bool TryResolvePlayerInputTarget(
        NodeExecContext& ctx,
        SessionId sessionId,
        NetId netId,
        PlayerInputTarget& out) noexcept
    {
        out = {};

        WorldRuntime* runtime = ctx.TryGetRuntime();
        ExecutionOps* ops = ctx.TryGetOps();
        const WorldId worldId = ctx.TryGetWorldId();
        if (runtime == nullptr || ops == nullptr || !worldId.IsValid() ||
            sessionId == 0 || !netId.IsValid())
        {
            FWLOG_WARN(kLogCategory,
                "TryResolvePlayerInputTarget failed at ctx preamble (sid=%u, netId=%u, runtime=%p, ops=%p, worldIdValid=%u)",
                sessionId,
                netId.GetRaw(),
                static_cast<const void*>(runtime),
                static_cast<const void*>(ops),
                worldId.IsValid() ? 1u : 0u);
            return false;
        }

        Entity entity = Entity::Null();
        if (!ops->TryResolveEntity(worldId, netId, entity) ||
            entity.IsNull())
        {
            FWLOG_WARN(kLogCategory,
                "TryResolvePlayerInputTarget failed: TryResolveEntity (sid=%u, netId=%u, worldId=%u)",
                sessionId,
                netId.GetRaw(),
                worldId.GetRaw());
            return false;
        }

        ECSView ecs = runtime->MakeView();
        PlayerControlIdentityComp* identity =
            ecs.GetMutableComponent<PlayerControlIdentityComp>(entity);
        PlayerNetworkTimingComp* networkTiming =
            ecs.GetMutableComponent<PlayerNetworkTimingComp>(entity);
        ActorInputComp* input =
            ecs.GetMutableComponent<ActorInputComp>(entity);
        if (identity == nullptr || input == nullptr)
        {
            FWLOG_WARN(kLogCategory,
                "TryResolvePlayerInputTarget failed: missing comp (sid=%u, netId=%u, entity=%u.%u, identity=%p, input=%p)",
                sessionId,
                netId.GetRaw(),
                entity.id,
                entity.generation,
                static_cast<const void*>(identity),
                static_cast<const void*>(input));
            return false;
        }

        if (identity->ownerSessionId != sessionId ||
            identity->netId != netId)
        {
            FWLOG_WARN(kLogCategory,
                "TryResolvePlayerInputTarget failed: identity mismatch (sid=%u, netId=%u, identity.ownerSessionId=%u, identity.netId=%u)",
                sessionId,
                netId.GetRaw(),
                identity->ownerSessionId,
                identity->netId.GetRaw());
            return false;
        }

        if (networkTiming != nullptr)
        {
            NetworkTimingSnapshot snapshot{};
            const PacketHandlerContext& svc = PacketHandlerContext::Get();
            if (svc.networkTiming != nullptr &&
                svc.networkTiming->TryGetSnapshot(sessionId, snapshot))
            {
                networkTiming->latestRttMs = snapshot.latestRttMs;
                networkTiming->smoothedRttMs = snapshot.smoothedRttMs;
                networkTiming->rttVarMs = snapshot.rttVarMs;
                networkTiming->arrivalJitterMs = snapshot.arrivalJitterMs;
                networkTiming->estimatedOneWayMs = snapshot.estimatedOneWayMs;
                networkTiming->sentProbeCount = snapshot.sentProbeCount;
                networkTiming->receivedProbeCount = snapshot.receivedProbeCount;
                networkTiming->rejectedProbeCount = snapshot.rejectedProbeCount;
                networkTiming->inputArrivalSampleCount =
                    snapshot.inputArrivalSampleCount;
                networkTiming->lastUpdatedFrame = runtime->FrameIndex();
                networkTiming->initialized = snapshot.initialized;
                networkTiming->arrivalJitterInitialized =
                    snapshot.arrivalJitterInitialized;
            }
        }

        out.entity = entity;
        out.identity = identity;
        out.networkTiming = networkTiming;
        out.input = input;
        out.runtime = runtime;
        return true;
    }

    bool TryResolvePlayerInputTarget(
        NodeExecContext& ctx,
        SessionId sessionId,
        const SessionFlowController& sessionFlow,
        PlayerInputTarget& out) noexcept
    {
        const NetId netId = sessionFlow.FindControlledNetId(sessionId);
        return TryResolvePlayerInputTarget(ctx, sessionId, netId, out);
    }

    void ApplyMoveInput(
        WorldRuntime& runtime,
        const Protocol::CS_MOVE_PACKET& packet,
        ActorInputComp& input) noexcept
    {
        input.move.inputX = GameplaySystemUtil::ClampFloat(
            static_cast<float>(packet.inputx()), -1.0f, 1.0f);
        input.move.inputZ = GameplaySystemUtil::ClampFloat(
            static_cast<float>(packet.inputz()), -1.0f, 1.0f);
        input.move.cameraYawRad = GameplaySystemUtil::WrapYaw(packet.yaw());
        input.move.wantsRun = packet.isrun();
        input.move.lastUpdatedFrame = runtime.FrameIndex();
    }

    void ApplyAbilityInput(
        WorldRuntime& runtime,
        PlayerAbilityInputType type,
        float directionX,
        float directionZ,
        ActorInputComp& input) noexcept
    {
        input.ability.directionX = directionX;
        input.ability.directionZ = directionZ;
        input.ability.requestedFrame = runtime.FrameIndex();
        input.ability.type = type;
        input.ability.clientAnimId = AnimationId::None;
        input.ability.clientNormalizedTime = 0.0f;
        input.ability.clientAbilityInstanceId = 0;
        input.ability.hasClientAnimationTiming = false;
    }

    AnimationId DecodeClientAnimationId(uint32_t rawAnimId) noexcept
    {
        if (rawAnimId == static_cast<uint32_t>(AnimationId::None) ||
            rawAnimId > static_cast<uint32_t>(AnimationId::Paladin_Death))
        {
            return AnimationId::None;
        }

        return static_cast<AnimationId>(rawAnimId);
    }

    void ApplyAttackInput(
        WorldRuntime& runtime,
        const Protocol::CS_ATTACK_PACKET& packet,
        ActorInputComp& input) noexcept
    {
        const PlayerAbilityInputType inputType =
            packet.attackinputtype() == Protocol::ATTACK_INPUT_HEAVY
            ? PlayerAbilityInputType::HeavyAttack
            : PlayerAbilityInputType::LightAttack;

        ApplyAbilityInput(
            runtime,
            inputType,
            packet.dirx(),
            packet.dirz(),
            input);

        const AnimationId clientAnimId =
            DecodeClientAnimationId(packet.clientanimid());
        const float clientNormalizedTime = packet.clientnormalizedtime();
        const uint32_t clientAbilityInstanceId =
            packet.clientabilityinstanceid();

        if (clientAnimId == AnimationId::None ||
            clientAbilityInstanceId == 0 ||
            !std::isfinite(clientNormalizedTime))
        {
            return;
        }

        input.ability.clientAnimId = clientAnimId;
        input.ability.clientNormalizedTime = GameplaySystemUtil::ClampFloat(
            clientNormalizedTime,
            0.0f,
            1.0f);
        input.ability.clientAbilityInstanceId = clientAbilityInstanceId;
        input.ability.hasClientAnimationTiming = true;
    }

    void ApplyGuardInput(
        WorldRuntime& runtime,
        const Protocol::CS_GUARD_PACKET& packet,
        ActorInputComp& input) noexcept
    {
        input.guard.isPressed = packet.input();
        input.guard.lastUpdatedFrame = runtime.FrameIndex();
    }

    // 방향 패킷 파싱 헬퍼
    template<typename T>
    bool ParseProto(const SendBuffer& buf, T& out)
    {
        const auto* header =
            reinterpret_cast<const PacketHeader*>(buf.data);
        return PacketFactory::Deserialize(*header, buf.data, &out);
    }

    // 세션이 이미 바인딩됐거나 진입 중이면 중복 로그인 거부
    bool IsLoginDuplicate(
        SessionId sessionId,
        const SessionFlowController& sessionFlow) noexcept
    {
        if (sessionFlow.HasBinding(sessionId))
            return true;

        const SessionFlow* const flow = sessionFlow.FindFlow(sessionId);
        if (flow != nullptr &&
            flow->stateId != SessionStateId::Connected &&
            flow->stateId != SessionStateId::Closing)
        {
            return true;
        }

        return false;
    }

    std::string_view TrimAscii(std::string_view value) noexcept
    {
        while (!value.empty() &&
            std::isspace(static_cast<unsigned char>(value.front())))
        {
            value.remove_prefix(1);
        }

        while (!value.empty() &&
            std::isspace(static_cast<unsigned char>(value.back())))
        {
            value.remove_suffix(1);
        }

        return value;
    }

    bool IsAllowedLoginIdChar(unsigned char ch) noexcept
    {
        return
            (ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '_' ||
            ch == '.' ||
            ch == '@' ||
            ch == '-';
    }

    bool NormalizeLoginId(
        std::string_view loginId,
        std::wstring& outNormalized) noexcept
    {
        outNormalized.clear();

        const std::string_view trimmed = TrimAscii(loginId);
        if (trimmed.size() < 4 || trimmed.size() > 64)
            return false;

        outNormalized.reserve(trimmed.size());
        for (const unsigned char ch : trimmed)
        {
            if (!IsAllowedLoginIdChar(ch))
                return false;

            const unsigned char lowered =
                static_cast<unsigned char>(std::tolower(ch));
            outNormalized.push_back(static_cast<wchar_t>(lowered));
        }

        return true;
    }

    bool ValidatePassword(std::string_view password) noexcept
    {
        if (password.size() < 8 || password.size() > 128)
            return false;

        return std::all_of(
            password.begin(),
            password.end(),
            [](unsigned char ch)
            {
                return ch >= 0x20 && ch <= 0x7e;
            });
    }

    uint32_t MapDBErrorToLoginFailReason(uint32_t errorCode) noexcept
    {
        switch (static_cast<DBCommonErrorCode>(errorCode))
        {
        case DBCommonErrorCode::Rejected:
        case DBCommonErrorCode::ConnectFail:
            return kLoginFailReasonDatabaseUnavailable;
        case DBCommonErrorCode::Timeout:
            return kLoginFailReasonTimeout;
        case DBCommonErrorCode::None:
            return kLoginFailReasonDatabaseError;
        case DBCommonErrorCode::QueryFail:
        case DBCommonErrorCode::Exception:
        default:
            return kLoginFailReasonDatabaseError;
        }
    }

    void StageAndCloseLoginFailure(
        PacketHandlerContext& svc,
        SessionId sessionId,
        uint32_t failReason) noexcept
    {
        if (svc.network != nullptr)
        {
            (void)ServerPacketStager::StageLoginFail(
                *svc.network,
                sessionId,
                failReason);
            (void)svc.network->RequestClose(
                sessionId,
                SessionCloseReason::ProtocolError);
        }

        if (svc.sessionFlow != nullptr)
        {
            LoginFailed failedCommand{};
            failedCommand.reason = failReason;
            (void)svc.sessionFlow->Dispatch(sessionId, failedCommand);
        }
    }

    void ReleaseDBCompletionPayload(uint64_t payloadKey) noexcept
    {
        if (payloadKey == 0)
            return;

        auto& svc = PacketHandlerContext::Get();
        if (svc.database != nullptr)
            svc.database->ResultStore().Drop(payloadKey);
    }

    bool CompleteDevStubLogin(
        PacketHandlerContext& svc,
        SessionId sessionId) noexcept
    {
        if (svc.framework == nullptr ||
            svc.sessionFlow == nullptr ||
            svc.network == nullptr)
        {
            return false;
        }

        const NetId controlledNetId = svc.framework->AllocateNetId();
        if (!controlledNetId.IsValid())
        {
            FWLOG_WARN(kLogCategory,
                "Login rejected: dev stub netId allocation failed (sid=%u)",
                sessionId);
            StageAndCloseLoginFailure(
                svc,
                sessionId,
                kLoginFailReasonEntryStartFailed);
            return true;
        }

        LoginSucceeded succeededCommand{};
        succeededCommand.controlledNetId = controlledNetId;
        const SessionFlowResult flowResult =
            svc.sessionFlow->Dispatch(sessionId, succeededCommand);
        if (!flowResult.Succeeded())
        {
            svc.framework->FreeNetId(controlledNetId);
            FWLOG_WARN(kLogCategory,
                "Dev stub login success rejected by flow (sid=%u, reason=%s)",
                sessionId,
                SessionFlowResultCodeName(flowResult.code));
            (void)svc.network->RequestClose(
                sessionId,
                SessionCloseReason::ProtocolError);
            return true;
        }

        if (!ServerPacketStager::StageLoginSuccess(
            *svc.network,
            sessionId,
            controlledNetId))
        {
            svc.framework->FreeNetId(controlledNetId);
            FWLOG_WARN(kLogCategory,
                "Dev stub login success packet stage failed (sid=%u, netId=%u)",
                sessionId,
                controlledNetId.GetRaw());
            (void)svc.network->RequestClose(
                sessionId,
                SessionCloseReason::ProtocolError);
        }

        return true;
    }

    void SubmitPartyCommand(PartyCommand command) noexcept
    {
        auto& svc = PacketHandlerContext::Get();
        if (svc.partyCommandQueue != nullptr)
        {
            svc.partyCommandQueue->Submit(std::move(command));
        }
    }

    const char* SessionFlowResultCodeName(SessionFlowResultCode code) noexcept
    {
        switch (code)
        {
        case SessionFlowResultCode::Accepted:            return "Accepted";
        case SessionFlowResultCode::InvalidSession:      return "InvalidSession";
        case SessionFlowResultCode::UnknownSession:      return "UnknownSession";
        case SessionFlowResultCode::InvalidFlowStage:    return "InvalidFlowStage";
        case SessionFlowResultCode::CommandTypeMismatch: return "CommandTypeMismatch";
        case SessionFlowResultCode::CloseRequested:      return "CloseRequested";
        default:                                         return "Unknown";
        }
    }

    const char* CharacterDataResultCodeName(CharacterDataResultCode code) noexcept
    {
        switch (code)
        {
        case CharacterDataResultCode::Success:                  return "Success";
        case CharacterDataResultCode::InvalidSession:           return "InvalidSession";
        case CharacterDataResultCode::InvalidCharacterId:       return "InvalidCharacterId";
        case CharacterDataResultCode::CharacterDefNotFound:     return "CharacterDefNotFound";
        case CharacterDataResultCode::CharacterIdNotPlayable:   return "CharacterIdNotPlayable";
        case CharacterDataResultCode::StartupWorldUnavailable:  return "StartupWorldUnavailable";
        case CharacterDataResultCode::WorldNotFound:            return "WorldNotFound";
        default:                                                return "Unknown";
        }
    }

    const char* CharacterSpawnResultCodeName(CharacterSpawnResultCode code) noexcept
    {
        switch (code)
        {
        case CharacterSpawnResultCode::Success:                 return "Success";
        case CharacterSpawnResultCode::InvalidSession:          return "InvalidSession";
        case CharacterSpawnResultCode::InvalidCharacterData:    return "InvalidCharacterData";
        case CharacterSpawnResultCode::InvalidReservedNetId:    return "InvalidReservedNetId";
        case CharacterSpawnResultCode::WorldNotFound:           return "WorldNotFound";
        case CharacterSpawnResultCode::EntityReserveFailed:     return "EntityReserveFailed";
        case CharacterSpawnResultCode::NetBindFailed:           return "NetBindFailed";
        case CharacterSpawnResultCode::DuplicatePendingSpawn:   return "DuplicatePendingSpawn";
        default:                                                return "Unknown";
        }
    }

    void ReleaseSendBufferPayload(uint64_t payloadKey) noexcept
    {
        if (payloadKey != 0)
        {
            SendBufferPtr payload{
                reinterpret_cast<SendBuffer*>(payloadKey)
            };
        }
    }

    void AddGameplayInputOrdering(DynamicTaskTypeDesc& desc)
    {
        desc.runsBefore.push_back(Tag<ResolvePlayerNetworkCompensationSystem>());
        desc.runsBefore.push_back(Tag<ResolveAbilityStateSystem>());
        desc.runsBefore.push_back(Tag<ResolveLocomotionStateSystem>());
        desc.runsBefore.push_back(Tag<ResolveDeathAndDespawnSystem>());
    }

    void AddGameplayInputAccesses(DynamicTaskTypeDesc& desc)
    {
        desc.accesses.push_back(
            ReadImmediate(ExternalRes<SessionFlowController>()));
        desc.accesses.push_back(
            ReadImmediate(ExternalRes<IWorldNetBindingResolver>()));
        desc.accesses.push_back(
            ReadImmediate(ComponentRes<PlayerControlIdentityComp>()));
        desc.accesses.push_back(
            WriteImmediate(ComponentRes<PlayerNetworkTimingComp>()));
        desc.accesses.push_back(
            WriteImmediate(ComponentRes<ActorInputComp>()));
    }

    void AddRespawnRequestOrdering(DynamicTaskTypeDesc& desc)
    {
        desc.runsBefore.push_back(Tag<ResolveDeathAndDespawnSystem>());
    }

    void AddRespawnRequestAccesses(DynamicTaskTypeDesc& desc)
    {
        desc.accesses.push_back(
            ReadImmediate(ExternalRes<SessionFlowController>()));
        desc.accesses.push_back(
            ReadImmediate(ExternalRes<IWorldNetBindingResolver>()));
        desc.accesses.push_back(
            ReadImmediate(ComponentRes<PlayerControlIdentityComp>()));
        desc.accesses.push_back(
            ReadImmediate(ComponentRes<CombatStatStateComp>()));
        desc.accesses.push_back(
            WriteImmediate(ComponentRes<PlayerDeathStateComp>()));
    }

    DynamicTaskTypeId RegisterPacketDynamicTask(
        DynamicTaskTypeRegistry& taskRegistry,
        ExecutionSourceRegistry& sourceRegistry,
        NetworkRuntime& network,
        PacketType packetType,
        ExecFn dispatchFn,
        const char* debugName,
        DynamicTaskTargetKind targetKind = DynamicTaskTargetKind::ExplicitScope,
        bool gameplayInput = false,
        bool respawnRequest = false)
    {
        DynamicTaskTypeDesc desc{};
        desc.tag               = InvalidExecTag;
        desc.debugName         = debugName;
        desc.defaultPhase      = ExecPhase::Simulate;
        desc.defaultLane       = ExecLane::Serial;
        desc.defaultTargetKind = targetKind;
        desc.dispatchFn        = dispatchFn;
        desc.payloadCleanupFn  = &ReleaseSendBufferPayload;

        if (gameplayInput)
        {
            AddGameplayInputOrdering(desc);
            AddGameplayInputAccesses(desc);
        }
        if (respawnRequest)
        {
            AddRespawnRequestOrdering(desc);
            AddRespawnRequestAccesses(desc);
        }

        const DynamicTaskTypeId typeId =
            taskRegistry.Register(desc, sourceRegistry);
        if (typeId != InvalidDynamicTaskTypeId)
        {
            network.RegisterPacketHandler(
                static_cast<uint16_t>(packetType),
                typeId);
            FWLOG_INFO(kLogCategory,
                "RegisterPacketDynamicTask ok (packetType=%u, typeId=%u, debugName=%s, targetKind=%d, gameplayInput=%u, respawnRequest=%u)",
                static_cast<uint32_t>(packetType),
                typeId,
                debugName != nullptr ? debugName : "(null)",
                static_cast<int>(targetKind),
                gameplayInput ? 1u : 0u,
                respawnRequest ? 1u : 0u);
        }
        else
        {
            FWLOG_ERROR(kLogCategory,
                "RegisterPacketDynamicTask FAILED (packetType=%u, debugName=%s, targetKind=%d, gameplayInput=%u, respawnRequest=%u)",
                static_cast<uint32_t>(packetType),
                debugName != nullptr ? debugName : "(null)",
                static_cast<int>(targetKind),
                gameplayInput ? 1u : 0u,
                respawnRequest ? 1u : 0u);
        }
        return typeId;
    }
}

void RegisterServerPacketHandlers(
    DynamicTaskTypeRegistry& taskRegistry,
    ExecutionSourceRegistry& sourceRegistry,
    NetworkRuntime& network,
    DynamicTaskTypeId& outDisconnectedTypeId)
{
    // CS_TIME_SYNC echo 처리는 NetworkTimingService(=글로벌 서비스)만 만지며
    // ECS 컴포넌트에 접근하지 않는다. 월드 그래프에 묶을 이유가 없고, 묶이면
    // 세션이 잠깐이라도 InGame이 아닐 때 패킷이 그대로 폐기되므로
    // ExplicitScope로 두어 항상 글로벌 스코프에서 처리한다.
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_TIME_SYNC,                &HandleTimeSyncPacket,              "Pkt_CS_TIME_SYNC",
        DynamicTaskTargetKind::ExplicitScope);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_LOGIN,                    &HandleLoginPacket,                  "Pkt_CS_LOGIN");
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_CHARACTER_SELECT,         &HandleCharacterSelectPacket,        "Pkt_CS_CHARACTER_SELECT");
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_MOVE,                     &HandleMovePacket,                   "Pkt_CS_MOVE",
        DynamicTaskTargetKind::SessionCurrentWorld, true);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_ATTACK,                   &HandleAttackPacket,                 "Pkt_CS_ATTACK",
        DynamicTaskTargetKind::SessionCurrentWorld, true);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_DODGE,                    &HandleDodgePacket,                  "Pkt_CS_DODGE",
        DynamicTaskTargetKind::SessionCurrentWorld, true);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_GUARD,                    &HandleGuardPacket,                  "Pkt_CS_GUARD",
        DynamicTaskTargetKind::SessionCurrentWorld, true);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_PARRY,                    &HandleParryPacket,                  "Pkt_CS_PARRY",
        DynamicTaskTargetKind::SessionCurrentWorld, true);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_USE_ITEM,                 &HandleUseItemPacket,                "Pkt_CS_USE_ITEM",
        DynamicTaskTargetKind::SessionCurrentWorld, true);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_WORLD_TRANSITION_REQUEST, &HandleWorldTransitionRequestPacket, "Pkt_CS_WORLD_TRANSITION_REQUEST");
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_WORLD_TRANSITION_READY,   &HandleWorldTransitionReadyPacket,   "Pkt_CS_WORLD_TRANSITION_READY");
    // FinalBoss 클리어 선택은 ECS input/월드 그래프와 무관하므로 ExplicitScope.
    // (sink->SubmitFinalClearPvpChoice 가 로직 스레드에서 ServerApp 상태를
    //  직접 갱신하는 점은 CS_WORLD_TRANSITION_READY 와 동일한 패턴이다.)
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_FINAL_CLEAR_CHOICE_SUBMIT, &HandleFinalClearChoiceSubmitPacket, "Pkt_CS_FINAL_CLEAR_CHOICE_SUBMIT",
        DynamicTaskTargetKind::ExplicitScope);
    // 엔딩 연출 완료 통지도 ECS/월드 그래프와 무관 → ExplicitScope (sink가 직접 처리).
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_FINAL_ENDING_CINEMATIC_DONE, &HandleFinalEndingCinematicDonePacket, "Pkt_CS_FINAL_ENDING_CINEMATIC_DONE",
        DynamicTaskTargetKind::ExplicitScope);
    // CS_PARTY_* 핸들러는 전부 PartyCommandQueue에 명령을 enqueue 하기만 하고
    // ECS 컴포넌트(ActorInputComp, PlayerNetworkTimingComp 등)나 월드 상태를
    // 직접 수정하지 않는다. 따라서 다음 두 조건이 모두 충족되어야 한다:
    //   1) gameplayInput=false: gameplay input과 동일한 access/ordering을
    //      걸지 않는다(파티 UI 핸들러가 CS_MOVE/CS_ATTACK과 같은 컴포넌트
    //      write 락을 들고 ResolvePlayerNetworkCompensationSystem 앞에
    //      직렬화되는 사고를 막는다).
    //   2) ExplicitScope: 세션이 InGame이 아니어도 파티 UI를 켜고 닫을 수
    //      있어야 하며, 월드 그래프에 묶일 이유도 없다.
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_PARTY_UI_OPENED,          &HandlePartyUiOpenedPacket,          "Pkt_CS_PARTY_UI_OPENED",
        DynamicTaskTargetKind::ExplicitScope);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_PARTY_UI_CLOSED,          &HandlePartyUiClosedPacket,          "Pkt_CS_PARTY_UI_CLOSED",
        DynamicTaskTargetKind::ExplicitScope);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_PARTY_LIST_REFRESH,       &HandlePartyListRefreshPacket,       "Pkt_CS_PARTY_LIST_REFRESH",
        DynamicTaskTargetKind::ExplicitScope);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_PARTY_CREATE,             &HandlePartyCreatePacket,            "Pkt_CS_PARTY_CREATE",
        DynamicTaskTargetKind::ExplicitScope);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_PARTY_JOIN_REQUEST,       &HandlePartyJoinRequestPacket,       "Pkt_CS_PARTY_JOIN_REQUEST",
        DynamicTaskTargetKind::ExplicitScope);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_PARTY_JOIN_ACCEPT,        &HandlePartyJoinAcceptPacket,        "Pkt_CS_PARTY_JOIN_ACCEPT",
        DynamicTaskTargetKind::ExplicitScope);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_PARTY_JOIN_REJECT,        &HandlePartyJoinRejectPacket,        "Pkt_CS_PARTY_JOIN_REJECT",
        DynamicTaskTargetKind::ExplicitScope);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_RESPAWN_REQUEST,           &HandleRespawnRequestPacket,         "Pkt_CS_RESPAWN_REQUEST",
        DynamicTaskTargetKind::SessionCurrentWorld, false, true);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_STAT_UI_OPENED,            &HandleStatUiOpenedPacket,           "Pkt_CS_STAT_UI_OPENED",
        DynamicTaskTargetKind::ExplicitScope);
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_TITLE_EQUIP_REQUEST,       &HandleTitleEquipRequestPacket,      "Pkt_CS_TITLE_EQUIP_REQUEST",
        DynamicTaskTargetKind::ExplicitScope);

    DynamicTaskTypeDesc loginAuthResultDesc{};
    loginAuthResultDesc.debugName = "DB_LoginAuthResult";
    loginAuthResultDesc.defaultPhase = ExecPhase::Simulate;
    loginAuthResultDesc.defaultLane = ExecLane::Serial;
    loginAuthResultDesc.defaultTargetKind = DynamicTaskTargetKind::ExplicitScope;
    loginAuthResultDesc.dispatchFn = &HandleLoginAuthResult;
    loginAuthResultDesc.payloadCleanupFn = &ReleaseDBCompletionPayload;
    g_loginAuthResultTaskTypeId =
        taskRegistry.Register(loginAuthResultDesc, sourceRegistry);

    DynamicTaskTypeDesc registerAccountResultDesc{};
    registerAccountResultDesc.debugName = "DB_RegisterAccountResult";
    registerAccountResultDesc.defaultPhase = ExecPhase::Simulate;
    registerAccountResultDesc.defaultLane = ExecLane::Serial;
    registerAccountResultDesc.defaultTargetKind = DynamicTaskTargetKind::ExplicitScope;
    registerAccountResultDesc.dispatchFn = &HandleRegisterAccountResult;
    registerAccountResultDesc.payloadCleanupFn = &ReleaseDBCompletionPayload;
    g_registerAccountResultTaskTypeId =
        taskRegistry.Register(registerAccountResultDesc, sourceRegistry);

    DynamicTaskTypeDesc desc{};
    desc.debugName    = "Evt_Disconnected";
    desc.defaultPhase = ExecPhase::Simulate;
    desc.defaultLane  = ExecLane::Serial;
    desc.defaultTargetKind = DynamicTaskTargetKind::SessionCurrentWorldOrExplicitScope;
    desc.dispatchFn   = &HandleDisconnectedEvent;

    outDisconnectedTypeId = taskRegistry.Register(desc, sourceRegistry);

    // -----------------------------------------------------------------------
    // 전투 통계 DB 완료 핸들러
    // -----------------------------------------------------------------------
    DynamicTaskTypeDesc incrementMonsterKillResultDesc{};
    incrementMonsterKillResultDesc.debugName       = "DB_IncrementMonsterKillCountResult";
    incrementMonsterKillResultDesc.defaultPhase    = ExecPhase::Simulate;
    incrementMonsterKillResultDesc.defaultLane     = ExecLane::Serial;
    incrementMonsterKillResultDesc.defaultTargetKind =
        DynamicTaskTargetKind::ExplicitScope;
    incrementMonsterKillResultDesc.dispatchFn      = &HandleIncrementMonsterKillCountResult;
    incrementMonsterKillResultDesc.payloadCleanupFn = &ReleaseDBCompletionPayload;
    g_incrementMonsterKillCountResultTaskTypeId =
        taskRegistry.Register(incrementMonsterKillResultDesc, sourceRegistry);

    DynamicTaskTypeDesc incrementDeathByMonsterResultDesc{};
    incrementDeathByMonsterResultDesc.debugName       = "DB_IncrementDeathByMonsterCountResult";
    incrementDeathByMonsterResultDesc.defaultPhase    = ExecPhase::Simulate;
    incrementDeathByMonsterResultDesc.defaultLane     = ExecLane::Serial;
    incrementDeathByMonsterResultDesc.defaultTargetKind =
        DynamicTaskTargetKind::ExplicitScope;
    incrementDeathByMonsterResultDesc.dispatchFn      = &HandleIncrementDeathByMonsterCountResult;
    incrementDeathByMonsterResultDesc.payloadCleanupFn = &ReleaseDBCompletionPayload;
    g_incrementDeathByMonsterCountResultTaskTypeId =
        taskRegistry.Register(incrementDeathByMonsterResultDesc, sourceRegistry);

    DynamicTaskTypeDesc unlockTitleResultDesc{};
    unlockTitleResultDesc.debugName       = "DB_UnlockTitleResult";
    unlockTitleResultDesc.defaultPhase    = ExecPhase::Simulate;
    unlockTitleResultDesc.defaultLane     = ExecLane::Serial;
    unlockTitleResultDesc.defaultTargetKind =
        DynamicTaskTargetKind::ExplicitScope;
    unlockTitleResultDesc.dispatchFn      = &HandleUnlockTitleResult;
    unlockTitleResultDesc.payloadCleanupFn = &ReleaseDBCompletionPayload;
    g_unlockTitleResultTaskTypeId =
        taskRegistry.Register(unlockTitleResultDesc, sourceRegistry);

    DynamicTaskTypeDesc getAccountTitlesResultDesc{};
    getAccountTitlesResultDesc.debugName =
        "DB_GetAccountTitlesResult";
    getAccountTitlesResultDesc.defaultPhase = ExecPhase::Simulate;
    getAccountTitlesResultDesc.defaultLane = ExecLane::Serial;
    getAccountTitlesResultDesc.defaultTargetKind =
        DynamicTaskTargetKind::ExplicitScope;
    getAccountTitlesResultDesc.dispatchFn =
        &HandleGetAccountTitlesResult;
    getAccountTitlesResultDesc.payloadCleanupFn =
        &ReleaseDBCompletionPayload;
    g_getAccountTitlesResultTaskTypeId =
        taskRegistry.Register(getAccountTitlesResultDesc, sourceRegistry);

    DynamicTaskTypeDesc setEquippedTitleResultDesc{};
    setEquippedTitleResultDesc.debugName =
        "DB_SetEquippedTitleResult";
    setEquippedTitleResultDesc.defaultPhase = ExecPhase::Simulate;
    setEquippedTitleResultDesc.defaultLane = ExecLane::Serial;
    setEquippedTitleResultDesc.defaultTargetKind =
        DynamicTaskTargetKind::ExplicitScope;
    setEquippedTitleResultDesc.dispatchFn =
        &HandleSetEquippedTitleResult;
    setEquippedTitleResultDesc.payloadCleanupFn =
        &ReleaseDBCompletionPayload;
    g_setEquippedTitleResultTaskTypeId =
        taskRegistry.Register(setEquippedTitleResultDesc, sourceRegistry);
}

ExecCallResult HandleLoginPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    Protocol::CS_LOGIN_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
    {
        FWLOG_WARN(kLogCategory, "Login rejected: malformed packet (sid=%u)", sessionId);
        (void)ServerPacketStager::StageLoginFail(
            *svc.network,
            sessionId,
            kLoginFailReasonMalformedPacket);
        (void)svc.network->RequestClose(sessionId, SessionCloseReason::ProtocolError);
        return ExecCallResult::Success;
    }

    if (svc.sessionFlow == nullptr ||
        svc.framework  == nullptr ||
        svc.network    == nullptr)
    {
        return ExecCallResult::Failed;
    }

    if (IsLoginDuplicate(sessionId, *svc.sessionFlow))
    {
        FWLOG_WARN(kLogCategory, "Duplicate login rejected (sid=%u)", sessionId);
        (void)ServerPacketStager::StageLoginFail(
            *svc.network,
            sessionId,
            kLoginFailReasonDuplicateLogin);
        return ExecCallResult::Success;
    }

    SessionFlowResult flowResult =
        svc.sessionFlow->Dispatch(sessionId, LoginRequested{});
    if (!flowResult.Succeeded())
    {
        FWLOG_WARN(kLogCategory,
            "Login rejected: invalid flow (sid=%u, reason=%s)",
            sessionId,
            SessionFlowResultCodeName(flowResult.code));
        (void)ServerPacketStager::StageLoginFail(
            *svc.network,
            sessionId,
            kLoginFailReasonDuplicateLogin);
        return ExecCallResult::Success;
    }

    std::wstring loginIdNormalized;
    if (!NormalizeLoginId(pkt.loginid(), loginIdNormalized) ||
        !ValidatePassword(pkt.password()))
    {
        FWLOG_WARN(kLogCategory,
            "Login rejected: invalid credentials format (sid=%u)",
            sessionId);
        StageAndCloseLoginFailure(
            svc,
            sessionId,
            kLoginFailReasonMalformedPacket);
        return ExecCallResult::Success;
    }

    if (svc.database == nullptr ||
        g_loginAuthResultTaskTypeId == InvalidDynamicTaskTypeId)
    {
        FWLOG_INFO(kLogCategory,
            "Login accepted by dev stub: database disabled (sid=%u)",
            sessionId);
        if (!CompleteDevStubLogin(svc, sessionId))
            return ExecCallResult::Failed;
        return ExecCallResult::Success;
    }

    DBCommandEnvelope envelope{};
    envelope.meta.sessionId = sessionId;
    envelope.meta.scopeId = ctx.scopeId;
    envelope.meta.requestFrameIndex = ResolveRequestFrameIndex(ctx);
    envelope.meta.completionTaskTypeId = g_loginAuthResultTaskTypeId;
    envelope.command = std::make_unique<LoginAuthCommand>(
        pkt.loginid(),
        std::move(loginIdNormalized),
        pkt.password());

    const DBRequestId requestId = svc.database->Submit(std::move(envelope));
    if (requestId == InvalidDBRequestId)
    {
        FWLOG_WARN(kLogCategory,
            "Login auth DB submit rejected (sid=%u)",
            sessionId);
    }

    return ExecCallResult::Success;
}

ExecCallResult HandleLoginAuthResult(NodeExecContext& ctx)
{
    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    if (svc.database == nullptr ||
        svc.sessionFlow == nullptr ||
        svc.framework == nullptr ||
        svc.network == nullptr)
    {
        return ExecCallResult::Failed;
    }

    const auto* inst =
        ctx.frame->dynamicTaskFrameTable->FindByNodeId(ctx.nodeId);
    if (inst == nullptr || inst->payloadKey == 0)
        return ExecCallResult::Failed;

    DBCompletion completion{};
    if (!svc.database->ResultStore().TakeCompletion(
        inst->payloadKey,
        completion))
    {
        FWLOG_WARN(kLogCategory,
            "Login auth result missing DB completion (sid=%u)",
            sessionId);
        return ExecCallResult::Success;
    }

    if (!completion.ok)
    {
        const uint32_t failReason =
            MapDBErrorToLoginFailReason(completion.errorCode);
        FWLOG_WARN(kLogCategory,
            "Login rejected: DB command failed (sid=%u, reason=%u, dbError=%u)",
            sessionId,
            failReason,
            completion.errorCode);
        StageAndCloseLoginFailure(svc, sessionId, failReason);
        return ExecCallResult::Success;
    }

    LoginAuthPayload payload{};
    if (completion.payloadKey == InvalidDBPayloadKey ||
        !svc.database->ResultStore().TakePayload(
            completion.payloadKey,
            payload))
    {
        FWLOG_WARN(kLogCategory,
            "Login rejected: DB payload missing (sid=%u)",
            sessionId);
        StageAndCloseLoginFailure(
            svc,
            sessionId,
            kLoginFailReasonDatabaseError);
        return ExecCallResult::Success;
    }

    if (payload.failReason != 0 || payload.accountId == 0)
    {
        if (payload.failReason ==
            static_cast<uint32_t>(LoginAuthFailReason::AccountNotFound))
        {
            if (svc.database == nullptr ||
                g_registerAccountResultTaskTypeId == InvalidDynamicTaskTypeId)
            {
                FWLOG_WARN(kLogCategory,
                    "Register rejected: database unavailable (sid=%u)",
                    sessionId);
                StageAndCloseLoginFailure(
                    svc,
                    sessionId,
                    kLoginFailReasonDatabaseUnavailable);
                return ExecCallResult::Success;
            }

            FWLOG_INFO(kLogCategory,
                "Account not found, attempting registration (sid=%u)",
                sessionId);

            DBCommandEnvelope envelope{};
            envelope.meta.sessionId            = sessionId;
            envelope.meta.scopeId              = ctx.scopeId;
            envelope.meta.requestFrameIndex    = ResolveRequestFrameIndex(ctx);
            envelope.meta.completionTaskTypeId = g_registerAccountResultTaskTypeId;
            envelope.command = std::make_unique<RegisterAccountCommand>(
                std::move(payload.loginId),
                std::move(payload.loginIdNormalized),
                std::move(payload.password));

            if (svc.database->Submit(std::move(envelope)) == InvalidDBRequestId)
            {
                FWLOG_WARN(kLogCategory,
                    "Register account DB submit rejected (sid=%u)",
                    sessionId);
                StageAndCloseLoginFailure(
                    svc,
                    sessionId,
                    kLoginFailReasonDatabaseUnavailable);
            }

            return ExecCallResult::Success;
        }

        const uint32_t failReason =
            payload.failReason != 0
            ? payload.failReason
            : kLoginFailReasonAuthRejected;
        FWLOG_WARN(kLogCategory,
            "Login rejected: auth failed (sid=%u, reason=%u)",
            sessionId,
            failReason);
        StageAndCloseLoginFailure(svc, sessionId, failReason);
        return ExecCallResult::Success;
    }

    if (svc.sessionFlow->HasAccountLogin(payload.accountId))
    {
        FWLOG_WARN(kLogCategory,
            "Login rejected: duplicate account login (sid=%u, accountId=%llu)",
            sessionId,
            static_cast<unsigned long long>(payload.accountId));
        StageAndCloseLoginFailure(
            svc,
            sessionId,
            kLoginFailReasonDuplicateLogin);
        return ExecCallResult::Success;
    }

    const NetId controlledNetId = svc.framework->AllocateNetId();
    if (!controlledNetId.IsValid())
    {
        FWLOG_WARN(kLogCategory, "Login rejected: netId allocation failed (sid=%u)", sessionId);
        StageAndCloseLoginFailure(
            svc,
            sessionId,
            kLoginFailReasonEntryStartFailed);
        return ExecCallResult::Success;
    }

    LoginSucceeded succeededCommand{};
    succeededCommand.accountId = payload.accountId;
    succeededCommand.controlledNetId = controlledNetId;
    const SessionFlowResult flowResult =
        svc.sessionFlow->Dispatch(sessionId, succeededCommand);
    if (!flowResult.Succeeded())
    {
        svc.framework->FreeNetId(controlledNetId);
        FWLOG_WARN(kLogCategory,
            "Login success rejected by flow (sid=%u, reason=%s)",
            sessionId,
            SessionFlowResultCodeName(flowResult.code));
        (void)svc.network->RequestClose(sessionId, SessionCloseReason::ProtocolError);
        return ExecCallResult::Success;
    }

    if (!ServerPacketStager::StageLoginSuccess(
        *svc.network,
        sessionId,
        controlledNetId))
    {
        svc.framework->FreeNetId(controlledNetId);
        FWLOG_WARN(kLogCategory,
            "Login success packet stage failed (sid=%u, netId=%u)",
            sessionId,
            controlledNetId.GetRaw());
        (void)svc.network->RequestClose(sessionId, SessionCloseReason::ProtocolError);
        return ExecCallResult::Success;
    }

    // DB에서 복구된 파티가 있으면 이 account를 해당 파티 멤버 슬롯에 재바인딩한다.
    SubmitPartyCommand(PartyCommand{
        .kind = PartyCommandKind::RebindRestoredMember,
        .actorSessionId = sessionId,
        .correlationId = payload.accountId
    });

    return ExecCallResult::Success;
}

ExecCallResult HandleRegisterAccountResult(NodeExecContext& ctx)
{
    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    if (svc.database   == nullptr ||
        svc.sessionFlow == nullptr ||
        svc.framework   == nullptr ||
        svc.network     == nullptr)
    {
        return ExecCallResult::Failed;
    }

    const auto* inst =
        ctx.frame->dynamicTaskFrameTable->FindByNodeId(ctx.nodeId);
    if (inst == nullptr || inst->payloadKey == 0)
        return ExecCallResult::Failed;

    DBCompletion completion{};
    if (!svc.database->ResultStore().TakeCompletion(
        inst->payloadKey,
        completion))
    {
        FWLOG_WARN(kLogCategory,
            "Register account result missing DB completion (sid=%u)",
            sessionId);
        return ExecCallResult::Success;
    }

    if (!completion.ok)
    {
        const uint32_t failReason =
            MapDBErrorToLoginFailReason(completion.errorCode);
        FWLOG_WARN(kLogCategory,
            "Register rejected: DB command failed (sid=%u, reason=%u, dbError=%u)",
            sessionId,
            failReason,
            completion.errorCode);
        StageAndCloseLoginFailure(svc, sessionId, failReason);
        return ExecCallResult::Success;
    }

    RegisterAccountPayload payload{};
    if (completion.payloadKey == InvalidDBPayloadKey ||
        !svc.database->ResultStore().TakePayload(
            completion.payloadKey,
            payload))
    {
        FWLOG_WARN(kLogCategory,
            "Register rejected: DB payload missing (sid=%u)",
            sessionId);
        StageAndCloseLoginFailure(svc, sessionId, kLoginFailReasonDatabaseError);
        return ExecCallResult::Success;
    }

    if (payload.failReason != 0 || payload.accountId == 0)
    {
        // DuplicateId: 동시 경쟁으로 이미 등록된 ID → AuthRejected로 응답 (재시도 유도)
        FWLOG_WARN(kLogCategory,
            "Register rejected: duplicate id or error (sid=%u, reason=%u)",
            sessionId,
            payload.failReason);
        StageAndCloseLoginFailure(svc, sessionId, kLoginFailReasonAuthRejected);
        return ExecCallResult::Success;
    }

    if (svc.sessionFlow->HasAccountLogin(payload.accountId))
    {
        FWLOG_WARN(kLogCategory,
            "Register rejected: duplicate account login (sid=%u, accountId=%llu)",
            sessionId,
            static_cast<unsigned long long>(payload.accountId));
        StageAndCloseLoginFailure(
            svc,
            sessionId,
            kLoginFailReasonDuplicateLogin);
        return ExecCallResult::Success;
    }

    FWLOG_INFO(kLogCategory,
        "New account registered and logged in (sid=%u, accountId=%llu)",
        sessionId,
        static_cast<unsigned long long>(payload.accountId));

    const NetId controlledNetId = svc.framework->AllocateNetId();
    if (!controlledNetId.IsValid())
    {
        FWLOG_WARN(kLogCategory,
            "Register login rejected: netId allocation failed (sid=%u)",
            sessionId);
        StageAndCloseLoginFailure(
            svc,
            sessionId,
            kLoginFailReasonEntryStartFailed);
        return ExecCallResult::Success;
    }

    LoginSucceeded succeededCommand{};
    succeededCommand.accountId       = payload.accountId;
    succeededCommand.controlledNetId = controlledNetId;
    const SessionFlowResult flowResult =
        svc.sessionFlow->Dispatch(sessionId, succeededCommand);
    if (!flowResult.Succeeded())
    {
        svc.framework->FreeNetId(controlledNetId);
        FWLOG_WARN(kLogCategory,
            "Register login success rejected by flow (sid=%u, reason=%s)",
            sessionId,
            SessionFlowResultCodeName(flowResult.code));
        (void)svc.network->RequestClose(sessionId, SessionCloseReason::ProtocolError);
        return ExecCallResult::Success;
    }

    if (!ServerPacketStager::StageLoginSuccess(
        *svc.network,
        sessionId,
        controlledNetId))
    {
        svc.framework->FreeNetId(controlledNetId);
        FWLOG_WARN(kLogCategory,
            "Register login success packet stage failed (sid=%u, netId=%u)",
            sessionId,
            controlledNetId.GetRaw());
        (void)svc.network->RequestClose(sessionId, SessionCloseReason::ProtocolError);
        return ExecCallResult::Success;
    }

    return ExecCallResult::Success;
}

ExecCallResult HandleTimeSyncPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
    {
        FWLOG_WARN(kLogCategory,
            "CS_TIME_SYNC entry: payload missing");
        return ExecCallResult::Failed;
    }

    const SessionId sessionId = ResolveSessionId(ctx);
    FWLOG_INFO(kLogCategory,
        "CS_TIME_SYNC entry (sid=%u, ctxWorldId=%u)",
        sessionId,
        ctx.TryGetWorldId().GetRaw());

    Protocol::CS_TIME_SYNC_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
    {
        FWLOG_WARN(kLogCategory,
            "CS_TIME_SYNC parse failed (sid=%u)",
            sessionId);
        return ExecCallResult::Success;
    }

    FWLOG_INFO(kLogCategory,
        "CS_TIME_SYNC parsed (sid=%u, probeSeq=%u, echoedServerSendTimeMs=%u)",
        sessionId,
        pkt.probeseq(),
        pkt.echoedserversendtimems());

    auto& svc = PacketHandlerContext::Get();
    if (svc.networkTiming != nullptr)
    {
        svc.networkTiming->HandleClientEcho(
            sessionId,
            pkt.probeseq(),
            pkt.echoedserversendtimems());
    }
    else
    {
        FWLOG_WARN(kLogCategory,
            "CS_TIME_SYNC ignored: networkTiming service is null (sid=%u)",
            sessionId);
    }

    return ExecCallResult::Success;
}

ExecCallResult HandleCharacterSelectPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    if (svc.sessionFlow    == nullptr ||
        svc.characterData  == nullptr ||
        svc.characterSpawn == nullptr ||
        svc.network        == nullptr)
    {
        return ExecCallResult::Failed;
    }

    Protocol::CS_CHARACTER_SELECT_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    const uint32_t rawCharacterId = pkt.characterid();
    if (rawCharacterId > std::numeric_limits<uint8_t>::max())
    {
        FWLOG_WARN(kLogCategory,
            "CharacterSelect rejected: characterId out of range (sid=%u, characterId=%u)",
            sessionId, rawCharacterId);
        (void)svc.network->RequestClose(sessionId, SessionCloseReason::ProtocolError);
        return ExecCallResult::Success;
    }

    const CharacterId characterId = static_cast<CharacterId>(rawCharacterId);
    CharacterSelectRequested selectCommand{};
    selectCommand.characterId = characterId;
    SessionFlowResult flowResult =
        svc.sessionFlow->Dispatch(sessionId, selectCommand);
    if (!flowResult.Succeeded())
    {
        FWLOG_WARN(kLogCategory,
            "CharacterSelect rejected by flow (sid=%u, characterId=%u, reason=%s)",
            sessionId,
            rawCharacterId,
            SessionFlowResultCodeName(flowResult.code));
        (void)svc.network->RequestClose(sessionId, SessionCloseReason::ProtocolError);
        return ExecCallResult::Success;
    }

    const CharacterDataResult dataResult =
        svc.characterData->ResolveCharacterSelect(sessionId, characterId);
    if (!dataResult.Succeeded())
    {
        FWLOG_WARN(kLogCategory,
            "CharacterSelect data rejected (sid=%u, characterId=%u, reason=%s)",
            sessionId,
            rawCharacterId,
            CharacterDataResultCodeName(dataResult.code));
        CharacterDataLoadFailed failedCommand{};
        failedCommand.reason = static_cast<uint32_t>(dataResult.code);
        (void)svc.sessionFlow->Dispatch(sessionId, failedCommand);
        (void)svc.network->RequestClose(sessionId, SessionCloseReason::ProtocolError);
        return ExecCallResult::Success;
    }

    CharacterDataLoaded loadedCommand{};
    loadedCommand.characterId = dataResult.characterId;
    loadedCommand.worldId     = dataResult.worldId;
    flowResult = svc.sessionFlow->Dispatch(sessionId, loadedCommand);
    if (!flowResult.Succeeded())
    {
        FWLOG_WARN(kLogCategory,
            "CharacterDataLoaded rejected by flow (sid=%u, reason=%s)",
            sessionId,
            SessionFlowResultCodeName(flowResult.code));
        (void)svc.network->RequestClose(sessionId, SessionCloseReason::ProtocolError);
        return ExecCallResult::Success;
    }

    const SessionFlow* const flow = svc.sessionFlow->FindFlow(sessionId);
    const NetId controlledNetId =
        flow != nullptr ? flow->controlledNetId : NetId::Invalid();
    const CharacterSpawnResult spawnResult =
        svc.characterSpawn->RequestCharacterSpawn(dataResult, controlledNetId);
    if (!spawnResult.Succeeded())
    {
        FWLOG_WARN(kLogCategory,
            "CharacterSelect spawn rejected (sid=%u, characterId=%u, reason=%s)",
            sessionId,
            rawCharacterId,
            CharacterSpawnResultCodeName(spawnResult.code));
        CharacterSpawnFailed failedCommand{};
        failedCommand.reason = static_cast<uint32_t>(spawnResult.code);
        (void)svc.sessionFlow->Dispatch(sessionId, failedCommand);
        (void)svc.network->RequestClose(sessionId, SessionCloseReason::ProtocolError);
        return ExecCallResult::Success;
    }

    FWLOG_DEBUG(kLogCategory,
        "CharacterSelect accepted (sid=%u, characterId=%u, worldId=%u, netId=%u)",
        sessionId,
        rawCharacterId,
        spawnResult.worldId.GetRaw(),
        spawnResult.netId.GetRaw());

    return ExecCallResult::Success;
}

ExecCallResult HandleMovePacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
    {
        FWLOG_WARN(kLogCategory,
            "CS_MOVE entry: payload missing");
        return ExecCallResult::Failed;
    }

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    FWLOG_INFO(kLogCategory,
        "CS_MOVE entry (sid=%u, ctxWorldId=%u, sessionFlow=%p, networkTiming=%p)",
        sessionId,
        ctx.TryGetWorldId().GetRaw(),
        static_cast<const void*>(svc.sessionFlow),
        static_cast<const void*>(svc.networkTiming));

    if (svc.sessionFlow == nullptr ||
        ResolveWorldRuntime(ctx, sessionId, *svc.sessionFlow) == nullptr)
    {
        FWLOG_INFO(kLogCategory,
            "CS_MOVE ignored before parse: no world runtime (sid=%u)",
            sessionId);
        return ExecCallResult::Success;
    }

    Protocol::CS_MOVE_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
    {
        FWLOG_WARN(kLogCategory,
            "CS_MOVE parse failed (sid=%u)",
            sessionId);
        return ExecCallResult::Success;
    }

    FWLOG_INFO(kLogCategory,
        "CS_MOVE parsed (sid=%u, inputX=%d, inputZ=%d, yaw=%.3f, isRun=%u)",
        sessionId,
        pkt.inputx(),
        pkt.inputz(),
        pkt.yaw(),
        pkt.isrun() ? 1u : 0u);

    if (svc.networkTiming != nullptr)
    {
        svc.networkTiming->RecordPeriodicInputArrival(sessionId);
    }

    PlayerInputTarget target{};
    if (!TryResolvePlayerInputTarget(ctx, sessionId, *svc.sessionFlow, target))
    {
        FWLOG_INFO(kLogCategory,
            "CS_MOVE ignored: input target not found (sid=%u)",
            sessionId);
        return ExecCallResult::Success;
    }

    ApplyMoveInput(*target.runtime, pkt, *target.input);
    FWLOG_INFO(kLogCategory,
        "CS_MOVE applied (sid=%u, entity=%u.%u, frame=%llu)",
        sessionId,
        target.entity.id,
        target.entity.generation,
        static_cast<unsigned long long>(target.runtime->FrameIndex()));
    return ExecCallResult::Success;
}

ExecCallResult HandleAttackPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
    {
        FWLOG_WARN(kLogCategory,
            "CS_ATTACK entry: payload missing");
        return ExecCallResult::Failed;
    }

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    FWLOG_INFO(kLogCategory,
        "CS_ATTACK entry (sid=%u, ctxWorldId=%u)",
        sessionId,
        ctx.TryGetWorldId().GetRaw());

    if (svc.sessionFlow == nullptr ||
        ResolveWorldRuntime(ctx, sessionId, *svc.sessionFlow) == nullptr)
    {
        FWLOG_INFO(kLogCategory,
            "CS_ATTACK ignored: no world runtime (sid=%u)",
            sessionId);
        return ExecCallResult::Success;
    }

    Protocol::CS_ATTACK_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
    {
        FWLOG_WARN(kLogCategory,
            "CS_ATTACK parse failed (sid=%u)",
            sessionId);
        return ExecCallResult::Success;
    }

    PlayerInputTarget target{};
    if (!TryResolvePlayerInputTarget(ctx, sessionId, *svc.sessionFlow, target))
    {
        FWLOG_INFO(kLogCategory,
            "CS_ATTACK ignored: input target not found (sid=%u)",
            sessionId);
        return ExecCallResult::Success;
    }

    ApplyAttackInput(*target.runtime, pkt, *target.input);
    FWLOG_INFO(kLogCategory,
        "CS_ATTACK applied (sid=%u, entity=%u.%u)",
        sessionId,
        target.entity.id,
        target.entity.generation);
    return ExecCallResult::Success;
}

ExecCallResult HandleDodgePacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    if (svc.sessionFlow == nullptr ||
        ResolveWorldRuntime(ctx, sessionId, *svc.sessionFlow) == nullptr)
    {
        return ExecCallResult::Success;
    }

    Protocol::CS_DODGE_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    PlayerInputTarget target{};
    if (!TryResolvePlayerInputTarget(ctx, sessionId, *svc.sessionFlow, target))
    {
        return ExecCallResult::Success;
    }

    ApplyAbilityInput(
        *target.runtime,
        PlayerAbilityInputType::Dodge,
        pkt.dirx(),
        pkt.dirz(),
        *target.input);
    return ExecCallResult::Success;
}

ExecCallResult HandleGuardPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    if (svc.sessionFlow == nullptr ||
        ResolveWorldRuntime(ctx, sessionId, *svc.sessionFlow) == nullptr)
    {
        return ExecCallResult::Success;
    }

    Protocol::CS_GUARD_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    PlayerInputTarget target{};
    if (!TryResolvePlayerInputTarget(ctx, sessionId, *svc.sessionFlow, target))
    {
        return ExecCallResult::Success;
    }

    ApplyGuardInput(*target.runtime, pkt, *target.input);
    return ExecCallResult::Success;
}

ExecCallResult HandleParryPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    if (svc.sessionFlow == nullptr ||
        ResolveWorldRuntime(ctx, sessionId, *svc.sessionFlow) == nullptr)
    {
        return ExecCallResult::Success;
    }

    Protocol::CS_PARRY_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    PlayerInputTarget target{};
    if (!TryResolvePlayerInputTarget(ctx, sessionId, *svc.sessionFlow, target))
    {
        return ExecCallResult::Success;
    }

    ApplyAbilityInput(
        *target.runtime,
        PlayerAbilityInputType::Parry,
        pkt.dirx(),
        pkt.dirz(),
        *target.input);
    return ExecCallResult::Success;
}

ExecCallResult HandleUseItemPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    if (svc.sessionFlow == nullptr ||
        ResolveWorldRuntime(ctx, sessionId, *svc.sessionFlow) == nullptr)
    {
        return ExecCallResult::Success;
    }

    Protocol::CS_USE_ITEM_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    PlayerInputTarget target{};
    if (!TryResolvePlayerInputTarget(ctx, sessionId, *svc.sessionFlow, target))
    {
        return ExecCallResult::Success;
    }

    ApplyAbilityInput(
        *target.runtime,
        PlayerAbilityInputType::UseItem,
        pkt.dirx(),
        pkt.dirz(),
        *target.input);
    return ExecCallResult::Success;
}

ExecCallResult HandleWorldTransitionRequestPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    if (svc.worldTransitionSink == nullptr)
    {
        if (svc.network != nullptr)
        {
            Protocol::CS_WORLD_TRANSITION_REQUEST_PACKET pkt{};
            if (ParseProto(*buf, pkt))
            {
                (void)ServerPacketStager::StageWorldTransitionRejectedPacket(
                    *svc.network,
                    sessionId,
                    pkt.requestid(),
                    kWorldTransitionRejectReasonServerUnavailable);
            }
        }
        return ExecCallResult::Success;
    }

    Protocol::CS_WORLD_TRANSITION_REQUEST_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    const TransferId transferId =
        svc.worldTransitionSink->RequestDemoWorldTransition(
            sessionId,
            pkt.requestid());
    if (transferId == 0)
    {
        FWLOG_WARN(kLogCategory,
            "WorldTransition request rejected (sid=%u, requestId=%u)",
            sessionId, pkt.requestid());

        if (svc.network != nullptr)
        {
            (void)ServerPacketStager::StageWorldTransitionRejectedPacket(
                *svc.network,
                sessionId,
                pkt.requestid(),
                kWorldTransitionRejectReasonRequestRejected);
        }
    }
    return ExecCallResult::Success;
}

ExecCallResult HandleWorldTransitionReadyPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    if (svc.worldTransitionSink == nullptr)
        return ExecCallResult::Success;

    Protocol::CS_WORLD_TRANSITION_READY_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    if (!svc.worldTransitionSink->MarkClientWorldTransitionReady(
            sessionId,
            static_cast<TransferId>(pkt.transferid())) &&
        svc.network != nullptr)
    {
        (void)svc.network->RequestClose(sessionId, SessionCloseReason::ProtocolError);
    }
    return ExecCallResult::Success;
}

ExecCallResult HandleFinalClearChoiceSubmitPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    if (svc.worldTransitionSink == nullptr)
        return ExecCallResult::Success;

    Protocol::CS_FINAL_CLEAR_CHOICE_SUBMIT_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    // 자격 없는 세션/만료된 voteId는 sink 내부에서 false로 무시된다.
    (void)svc.worldTransitionSink->SubmitFinalClearPvpChoice(
        sessionId,
        pkt.voteid(),
        pkt.choosepvp());
    return ExecCallResult::Success;
}

ExecCallResult HandleFinalEndingCinematicDonePacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    if (svc.worldTransitionSink == nullptr)
        return ExecCallResult::Success;

    Protocol::CS_FINAL_ENDING_CINEMATIC_DONE_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    // 해당 파티의 대기 중인 엔딩 전이가 없으면 sink 내부에서 false로 무시된다.
    (void)svc.worldTransitionSink->SubmitFinalEndingCinematicDone(
        sessionId,
        static_cast<uint32_t>(pkt.context()));
    return ExecCallResult::Success;
}

ExecCallResult HandlePartyUiOpenedPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
    {
        FWLOG_WARN(kLogCategory,
            "CS_PARTY_UI_OPENED entry: payload missing");
        return ExecCallResult::Failed;
    }

    const SessionId sessionId = ResolveSessionId(ctx);
    FWLOG_INFO(kLogCategory,
        "CS_PARTY_UI_OPENED entry (sid=%u, ctxWorldId=%u)",
        sessionId,
        ctx.TryGetWorldId().GetRaw());

    Protocol::CS_PARTY_UI_OPENED_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
    {
        FWLOG_WARN(kLogCategory,
            "CS_PARTY_UI_OPENED parse failed (sid=%u)",
            sessionId);
        return ExecCallResult::Success;
    }

    FWLOG_INFO(kLogCategory,
        "CS_PARTY_UI_OPENED parsed (sid=%u, clientRequestId=%u)",
        sessionId,
        pkt.clientrequestid());

    SubmitPartyCommand(PartyCommand{
        .kind = PartyCommandKind::UiOpened,
        .actorSessionId = sessionId,
        .clientRequestId = pkt.clientrequestid()
    });
    FWLOG_INFO(kLogCategory,
        "CS_PARTY_UI_OPENED submitted command (sid=%u, clientRequestId=%u)",
        sessionId,
        pkt.clientrequestid());
    return ExecCallResult::Success;
}

ExecCallResult HandlePartyUiClosedPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
    {
        FWLOG_WARN(kLogCategory,
            "CS_PARTY_UI_CLOSED entry: payload missing");
        return ExecCallResult::Failed;
    }

    const SessionId sessionId = ResolveSessionId(ctx);
    FWLOG_INFO(kLogCategory,
        "CS_PARTY_UI_CLOSED entry (sid=%u, ctxWorldId=%u)",
        sessionId,
        ctx.TryGetWorldId().GetRaw());

    Protocol::CS_PARTY_UI_CLOSED_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
    {
        FWLOG_WARN(kLogCategory,
            "CS_PARTY_UI_CLOSED parse failed (sid=%u)",
            sessionId);
        return ExecCallResult::Success;
    }

    SubmitPartyCommand(PartyCommand{
        .kind = PartyCommandKind::UiClosed,
        .actorSessionId = sessionId,
        .clientRequestId = pkt.clientrequestid()
    });
    FWLOG_INFO(kLogCategory,
        "CS_PARTY_UI_CLOSED submitted command (sid=%u, clientRequestId=%u)",
        sessionId,
        pkt.clientrequestid());
    return ExecCallResult::Success;
}

ExecCallResult HandlePartyListRefreshPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    Protocol::CS_PARTY_LIST_REFRESH_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    SubmitPartyCommand(PartyCommand{
        .kind = PartyCommandKind::ListRefresh,
        .actorSessionId = ResolveSessionId(ctx),
        .clientRequestId = pkt.clientrequestid()
    });
    return ExecCallResult::Success;
}

ExecCallResult HandlePartyCreatePacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    const SessionId sessionId = ResolveSessionId(ctx);
    Protocol::CS_PARTY_CREATE_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
    {
        FWLOG_WARN(kLogCategory,
            "CS_PARTY_CREATE parse failed (sid=%u)",
            sessionId);
        return ExecCallResult::Success;
    }

    FWLOG_INFO(kLogCategory,
        "CS_PARTY_CREATE parsed (sid=%u, clientRequestId=%u)",
        sessionId,
        pkt.clientrequestid());

    SubmitPartyCommand(PartyCommand{
        .kind = PartyCommandKind::CreateParty,
        .actorSessionId = sessionId,
        .clientRequestId = pkt.clientrequestid()
    });
    FWLOG_INFO(kLogCategory,
        "CS_PARTY_CREATE submitted command (sid=%u, clientRequestId=%u)",
        sessionId,
        pkt.clientrequestid());
    return ExecCallResult::Success;
}

ExecCallResult HandlePartyJoinRequestPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    Protocol::CS_PARTY_JOIN_REQUEST_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    FWLOG_INFO(kLogCategory,
        "CS_PARTY_JOIN_REQUEST parsed (sid=%u, partyId=%llu, clientRequestId=%u)",
        ResolveSessionId(ctx),
        static_cast<unsigned long long>(pkt.partyid()),
        pkt.clientrequestid());

    SubmitPartyCommand(PartyCommand{
        .kind = PartyCommandKind::RequestJoin,
        .actorSessionId = ResolveSessionId(ctx),
        .partyId = static_cast<PartyId>(pkt.partyid()),
        .clientRequestId = pkt.clientrequestid()
    });
    return ExecCallResult::Success;
}

ExecCallResult HandlePartyJoinAcceptPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    Protocol::CS_PARTY_JOIN_ACCEPT_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    SubmitPartyCommand(PartyCommand{
        .kind = PartyCommandKind::AcceptJoinRequest,
        .actorSessionId = ResolveSessionId(ctx),
        .requestId = static_cast<PartyRequestId>(pkt.joinrequestid()),
        .clientRequestId = pkt.clientrequestid()
    });
    return ExecCallResult::Success;
}

ExecCallResult HandlePartyJoinRejectPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    Protocol::CS_PARTY_JOIN_REJECT_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    SubmitPartyCommand(PartyCommand{
        .kind = PartyCommandKind::RejectJoinRequest,
        .actorSessionId = ResolveSessionId(ctx),
        .requestId = static_cast<PartyRequestId>(pkt.joinrequestid()),
        .clientRequestId = pkt.clientrequestid()
    });
    return ExecCallResult::Success;
}

ExecCallResult HandleRespawnRequestPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
    {
        FWLOG_WARN(kLogCategory, "CS_RESPAWN_REQUEST entry: payload missing");
        return ExecCallResult::Failed;
    }

    Protocol::CS_RESPAWN_REQUEST_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
    {
        FWLOG_WARN(kLogCategory, "CS_RESPAWN_REQUEST parse failed");
        return ExecCallResult::Success;
    }

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);
    if (svc.sessionFlow == nullptr ||
        ResolveWorldRuntime(ctx, sessionId, *svc.sessionFlow) == nullptr)
    {
        FWLOG_INFO(kLogCategory,
            "CS_RESPAWN_REQUEST ignored: no world runtime (sid=%u, requestId=%u)",
            sessionId,
            pkt.clientrequestid());
        return ExecCallResult::Success;
    }

    const NetId netId = svc.sessionFlow->FindControlledNetId(sessionId);
    WorldRuntime* const runtime = ctx.TryGetRuntime();
    ExecutionOps* const ops = ctx.TryGetOps();
    const WorldId worldId = ctx.TryGetWorldId();
    Entity entity = Entity::Null();
    if (runtime == nullptr ||
        ops == nullptr ||
        !netId.IsValid() ||
        !ops->TryResolveEntity(worldId, netId, entity) ||
        entity.IsNull())
    {
        FWLOG_INFO(kLogCategory,
            "CS_RESPAWN_REQUEST ignored: player target missing (sid=%u, requestId=%u)",
            sessionId,
            pkt.clientrequestid());
        return ExecCallResult::Success;
    }

    const WorldDef* const worldDef = runtime->GetDef();
    if (worldDef == nullptr ||
        !PlayerDeathStatePolicy::IsRespawnWorld(worldDef->id))
    {
        FWLOG_INFO(kLogCategory,
            "CS_RESPAWN_REQUEST ignored: unsupported world (sid=%u, requestId=%u, worldDefId=%u)",
            sessionId,
            pkt.clientrequestid(),
            worldDef != nullptr
                ? static_cast<uint32_t>(worldDef->id)
                : static_cast<uint32_t>(WorldDefId::None));
        return ExecCallResult::Success;
    }

    ECSView ecs = runtime->MakeView();
    const PlayerControlIdentityComp* const player =
        ecs.GetComponent<PlayerControlIdentityComp>(entity);
    const CombatStatStateComp* const stats =
        ecs.GetComponent<CombatStatStateComp>(entity);
    PlayerDeathStateComp* const deathState =
        ecs.GetMutableComponent<PlayerDeathStateComp>(entity);
    if (player == nullptr ||
        player->ownerSessionId != sessionId ||
        player->netId != netId ||
        stats == nullptr ||
        stats->currentHp > 0 ||
        deathState == nullptr ||
        !PlayerDeathStatePolicy::CanLatchRespawnRequest(*deathState))
    {
        FWLOG_INFO(kLogCategory,
            "CS_RESPAWN_REQUEST rejected (sid=%u, requestId=%u, hp=%d, deathState=%u, ownerSid=%u, playerNetId=%u, controlledNetId=%u)",
            sessionId,
            pkt.clientrequestid(),
            stats != nullptr ? stats->currentHp : -1,
            deathState != nullptr
                ? static_cast<uint32_t>(deathState->state)
                : std::numeric_limits<uint32_t>::max(),
            player != nullptr ? player->ownerSessionId : 0,
            player != nullptr ? player->netId.GetRaw() : 0,
            netId.GetRaw());
        return ExecCallResult::Success;
    }

    deathState->respawnRequested = true;
    FWLOG_INFO(kLogCategory,
        "CS_RESPAWN_REQUEST accepted (sid=%u, requestId=%u, entity=%u.%u, deathState=%u)",
        sessionId,
        pkt.clientrequestid(),
        entity.id,
        entity.generation,
        static_cast<uint32_t>(deathState->state));
    return ExecCallResult::Success;
}

ExecCallResult HandleStatUiOpenedPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    Protocol::CS_STAT_UI_OPENED_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);
    const SessionFlow* const flow =
        svc.sessionFlow != nullptr
        ? svc.sessionFlow->FindFlow(sessionId)
        : nullptr;

    if (svc.database == nullptr ||
        flow == nullptr ||
        flow->accountId == 0)
    {
        if (svc.network != nullptr)
        {
            (void)ServerPacketStager::StageStatUiBootstrapPacket(
                *svc.network,
                sessionId,
                pkt.clientrequestid(),
                {},
                InvalidTitleId);
        }
        return ExecCallResult::Success;
    }

    DBCommandEnvelope envelope{};
    envelope.meta.sessionId = sessionId;
    envelope.meta.clientRequestId = pkt.clientrequestid();
    envelope.meta.scopeId = ctx.scopeId;
    envelope.meta.requestFrameIndex = ResolveRequestFrameIndex(ctx);
    envelope.meta.completionTaskTypeId =
        g_getAccountTitlesResultTaskTypeId;
    envelope.command = std::make_unique<GetAccountTitlesCommand>(
        flow->accountId,
        sessionId,
        pkt.clientrequestid());
    (void)svc.database->Submit(std::move(envelope));
    return ExecCallResult::Success;
}

ExecCallResult HandleTitleEquipRequestPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    Protocol::CS_TITLE_EQUIP_REQUEST_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);
    const uint32_t titleIdRaw = pkt.titleid();
    const bool titleIdInRange =
        titleIdRaw <= static_cast<uint32_t>(
            std::numeric_limits<int16_t>::max());
    const TitleId titleId = titleIdInRange
        ? static_cast<TitleId>(titleIdRaw)
        : InvalidTitleId;
    const bool titleIsDefined =
        titleId == InvalidTitleId ||
        GameDataCatalog::Current().Titles().Find(titleId) != nullptr;

    if (!titleIdInRange || !titleIsDefined)
    {
        if (svc.network != nullptr)
        {
            (void)ServerPacketStager::StageTitleEquipResultPacket(
                *svc.network,
                sessionId,
                pkt.clientrequestid(),
                false,
                InvalidTitleId,
                kTitleEquipReasonInvalidTitle);
        }
        return ExecCallResult::Success;
    }

    const SessionFlow* const flow =
        svc.sessionFlow != nullptr
        ? svc.sessionFlow->FindFlow(sessionId)
        : nullptr;
    if (svc.database == nullptr ||
        flow == nullptr ||
        flow->accountId == 0)
    {
        if (svc.network != nullptr)
        {
            (void)ServerPacketStager::StageTitleEquipResultPacket(
                *svc.network,
                sessionId,
                pkt.clientrequestid(),
                false,
                InvalidTitleId,
                kTitleEquipReasonDatabaseError);
        }
        return ExecCallResult::Success;
    }

    DBCommandEnvelope envelope{};
    envelope.meta.sessionId = sessionId;
    envelope.meta.clientRequestId = pkt.clientrequestid();
    envelope.meta.scopeId = ctx.scopeId;
    envelope.meta.requestFrameIndex = ResolveRequestFrameIndex(ctx);
    envelope.meta.completionTaskTypeId =
        g_setEquippedTitleResultTaskTypeId;
    envelope.command = std::make_unique<SetEquippedTitleCommand>(
        flow->accountId,
        titleId,
        sessionId,
        pkt.clientrequestid());
    (void)svc.database->Submit(std::move(envelope));
    return ExecCallResult::Success;
}

ExecCallResult HandleDisconnectedEvent(NodeExecContext& ctx)
{
    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);
    SessionCloseReason reason = SessionCloseReason::RemoteClosed;

    if (const auto* inst =
        ctx.frame->dynamicTaskFrameTable->FindByNodeId(ctx.nodeId))
    {
        if (inst->payloadKey >
            static_cast<uint64_t>(SessionCloseReason::None) &&
            inst->payloadKey <=
            static_cast<uint64_t>(SessionCloseReason::ServerShutdown))
        {
            reason = static_cast<SessionCloseReason>(inst->payloadKey);
        }
    }

    if (svc.sessionSystem == nullptr)
        return ExecCallResult::Failed;

    svc.sessionSystem->HandleSessionDisconnected(
        sessionId,
        reason,
        ctx.TryGetWorldId());
    return ExecCallResult::Success;
}

// ---------------------------------------------------------------------------
// HandleIncrementMonsterKillCountResult
//
// SP 반환값(new_kill_count)을 바탕으로 TitleDefRegistry를 검사하여
// 달성 조건을 충족하는 칭호마다 UnlockTitleCommand를 발행한다.
// ---------------------------------------------------------------------------
ExecCallResult HandleIncrementMonsterKillCountResult(NodeExecContext& ctx)
{
    static constexpr const char* kCategory = "CombatStat";

    auto& svc = PacketHandlerContext::Get();
    if (svc.database == nullptr)
        return ExecCallResult::Failed;

    const auto* inst =
        ctx.frame->dynamicTaskFrameTable->FindByNodeId(ctx.nodeId);
    if (inst == nullptr || inst->payloadKey == 0)
        return ExecCallResult::Failed;

    DBCompletion completion{};
    if (!svc.database->ResultStore().TakeCompletion(inst->payloadKey, completion))
    {
        FWLOG_WARN(kCategory, "IncrementMonsterKillCount completion missing");
        return ExecCallResult::Success;
    }

    if (!completion.ok)
    {
        FWLOG_WARN(kCategory,
            "IncrementMonsterKillCount failed (dbError=%u)",
            completion.errorCode);
        return ExecCallResult::Success;
    }

    IncrementMonsterKillCountPayload payload{};
    if (completion.payloadKey == InvalidDBPayloadKey ||
        !svc.database->ResultStore().TakePayload(completion.payloadKey, payload))
    {
        FWLOG_WARN(kCategory, "IncrementMonsterKillCount payload missing");
        return ExecCallResult::Success;
    }

    // TitleDefRegistry 에서 MonsterKill 조건을 만족하는 칭호를 찾아 잠금 해제 커맨드 발행.
    for (const TitleDef& titleDef :
        GameDataCatalog::Current().Titles().GetAll())
    {
        if (titleDef.conditionType != TitleConditionType::MonsterKill)
            continue;
        if (static_cast<uint8_t>(titleDef.characterId) != payload.characterTypeId)
            continue;
        if (payload.newKillCount < titleDef.requiredCount)
            continue;

        DBCommandEnvelope envelope{};
        envelope.meta.sessionId            = payload.sessionId;
        envelope.meta.scopeId              = ctx.scopeId;
        envelope.meta.requestFrameIndex    = ResolveRequestFrameIndex(ctx);
        envelope.meta.completionTaskTypeId = g_unlockTitleResultTaskTypeId;
        envelope.command = std::make_unique<UnlockTitleCommand>(
            payload.accountId,
            titleDef.id,
            payload.sessionId);

        (void)svc.database->Submit(std::move(envelope));
    }

    return ExecCallResult::Success;
}

// ---------------------------------------------------------------------------
// HandleIncrementDeathByMonsterCountResult
// ---------------------------------------------------------------------------
ExecCallResult HandleIncrementDeathByMonsterCountResult(NodeExecContext& ctx)
{
    static constexpr const char* kCategory = "CombatStat";

    auto& svc = PacketHandlerContext::Get();
    if (svc.database == nullptr)
        return ExecCallResult::Failed;

    const auto* inst =
        ctx.frame->dynamicTaskFrameTable->FindByNodeId(ctx.nodeId);
    if (inst == nullptr || inst->payloadKey == 0)
        return ExecCallResult::Failed;

    DBCompletion completion{};
    if (!svc.database->ResultStore().TakeCompletion(inst->payloadKey, completion))
    {
        FWLOG_WARN(kCategory, "IncrementDeathByMonsterCount completion missing");
        return ExecCallResult::Success;
    }

    if (!completion.ok)
    {
        FWLOG_WARN(kCategory,
            "IncrementDeathByMonsterCount failed (dbError=%u)",
            completion.errorCode);
        return ExecCallResult::Success;
    }

    IncrementDeathByMonsterCountPayload payload{};
    if (completion.payloadKey == InvalidDBPayloadKey ||
        !svc.database->ResultStore().TakePayload(completion.payloadKey, payload))
    {
        FWLOG_WARN(kCategory, "IncrementDeathByMonsterCount payload missing");
        return ExecCallResult::Success;
    }

    // TitleDefRegistry 에서 DeathByMonster 조건을 만족하는 칭호를 찾아 잠금 해제 커맨드 발행.
    for (const TitleDef& titleDef :
        GameDataCatalog::Current().Titles().GetAll())
    {
        if (titleDef.conditionType != TitleConditionType::DeathByMonster)
            continue;
        if (static_cast<uint8_t>(titleDef.characterId) != payload.characterTypeId)
            continue;
        if (payload.newDeathCount < titleDef.requiredCount)
            continue;

        DBCommandEnvelope envelope{};
        envelope.meta.sessionId            = payload.sessionId;
        envelope.meta.scopeId              = ctx.scopeId;
        envelope.meta.requestFrameIndex    = ResolveRequestFrameIndex(ctx);
        envelope.meta.completionTaskTypeId = g_unlockTitleResultTaskTypeId;
        envelope.command = std::make_unique<UnlockTitleCommand>(
            payload.accountId,
            titleDef.id,
            payload.sessionId);

        (void)svc.database->Submit(std::move(envelope));
    }

    return ExecCallResult::Success;
}

// ---------------------------------------------------------------------------
// HandleUnlockTitleResult
//
// 칭호 잠금 해제 결과를 처리한다.
// newlyUnlocked == true 이면 최초 획득이므로 클라이언트 알림이 필요하나,
// 알림 패킷 인프라는 추후 구현 예정이므로 현재는 로그만 기록한다.
// ---------------------------------------------------------------------------
ExecCallResult HandleUnlockTitleResult(NodeExecContext& ctx)
{
    static constexpr const char* kCategory = "CombatStat";

    auto& svc = PacketHandlerContext::Get();
    if (svc.database == nullptr)
        return ExecCallResult::Failed;

    const auto* inst =
        ctx.frame->dynamicTaskFrameTable->FindByNodeId(ctx.nodeId);
    if (inst == nullptr || inst->payloadKey == 0)
        return ExecCallResult::Failed;

    DBCompletion completion{};
    if (!svc.database->ResultStore().TakeCompletion(inst->payloadKey, completion))
    {
        FWLOG_WARN(kCategory, "UnlockTitle completion missing");
        return ExecCallResult::Success;
    }

    if (!completion.ok)
    {
        FWLOG_WARN(kCategory,
            "UnlockTitle failed (dbError=%u)",
            completion.errorCode);
        return ExecCallResult::Success;
    }

    UnlockTitlePayload payload{};
    if (completion.payloadKey == InvalidDBPayloadKey ||
        !svc.database->ResultStore().TakePayload(completion.payloadKey, payload))
    {
        FWLOG_WARN(kCategory, "UnlockTitle payload missing");
        return ExecCallResult::Success;
    }

    if (payload.newlyUnlocked)
    {
        FWLOG_INFO(kCategory,
            "Title newly unlocked (titleId=%u, sid=%u) — notification TODO",
            static_cast<unsigned>(payload.titleId),
            payload.sessionId);
        // TODO: 클라이언트에 SC_TITLE_UNLOCKED 패킷 전송
    }

    return ExecCallResult::Success;
}

ExecCallResult HandleGetAccountTitlesResult(NodeExecContext& ctx)
{
    static constexpr const char* kCategory = "Title";

    auto& svc = PacketHandlerContext::Get();
    if (svc.database == nullptr)
        return ExecCallResult::Failed;

    const auto* inst =
        ctx.frame->dynamicTaskFrameTable->FindByNodeId(ctx.nodeId);
    if (inst == nullptr || inst->payloadKey == 0)
        return ExecCallResult::Failed;

    DBCompletion completion{};
    if (!svc.database->ResultStore().TakeCompletion(
        inst->payloadKey,
        completion))
    {
        FWLOG_WARN(kCategory, "GetAccountTitles completion missing");
        return ExecCallResult::Success;
    }

    if (!completion.ok)
    {
        FWLOG_WARN(kCategory,
            "GetAccountTitles failed (sid=%u, dbError=%u)",
            completion.sessionId,
            completion.errorCode);
        if (svc.network != nullptr)
        {
            (void)ServerPacketStager::StageStatUiBootstrapPacket(
                *svc.network,
                completion.sessionId,
                completion.clientRequestId,
                {},
                InvalidTitleId);
        }
        return ExecCallResult::Success;
    }

    GetAccountTitlesPayload payload{};
    if (completion.payloadKey == InvalidDBPayloadKey ||
        !svc.database->ResultStore().TakePayload(
            completion.payloadKey,
            payload))
    {
        FWLOG_WARN(kCategory, "GetAccountTitles payload missing");
        return ExecCallResult::Success;
    }

    const SessionFlow* const flow =
        svc.sessionFlow != nullptr
        ? svc.sessionFlow->FindFlow(completion.sessionId)
        : nullptr;
    if (flow == nullptr || flow->accountId != payload.accountId)
    {
        FWLOG_WARN(kCategory,
            "GetAccountTitles stale completion dropped (sid=%u)",
            completion.sessionId);
        return ExecCallResult::Success;
    }

    std::vector<TitleId> validTitleIds;
    validTitleIds.reserve(payload.ownedTitleIds.size());
    for (const TitleId titleId : payload.ownedTitleIds)
    {
        if (titleId != InvalidTitleId &&
            GameDataCatalog::Current().Titles().Find(titleId) != nullptr)
        {
            validTitleIds.push_back(titleId);
        }
    }

    std::sort(validTitleIds.begin(), validTitleIds.end());
    validTitleIds.erase(
        std::unique(validTitleIds.begin(), validTitleIds.end()),
        validTitleIds.end());

    const bool equippedIsOwned =
        payload.equippedTitleId == InvalidTitleId ||
        std::binary_search(
            validTitleIds.begin(),
            validTitleIds.end(),
            payload.equippedTitleId);
    const TitleId equippedTitleId = equippedIsOwned
        ? payload.equippedTitleId
        : InvalidTitleId;

    if (svc.network != nullptr)
    {
        (void)ServerPacketStager::StageStatUiBootstrapPacket(
            *svc.network,
            completion.sessionId,
            completion.clientRequestId,
            validTitleIds,
            equippedTitleId);
    }
    return ExecCallResult::Success;
}

ExecCallResult HandleSetEquippedTitleResult(NodeExecContext& ctx)
{
    static constexpr const char* kCategory = "Title";

    auto& svc = PacketHandlerContext::Get();
    if (svc.database == nullptr)
        return ExecCallResult::Failed;

    const auto* inst =
        ctx.frame->dynamicTaskFrameTable->FindByNodeId(ctx.nodeId);
    if (inst == nullptr || inst->payloadKey == 0)
        return ExecCallResult::Failed;

    DBCompletion completion{};
    if (!svc.database->ResultStore().TakeCompletion(
        inst->payloadKey,
        completion))
    {
        FWLOG_WARN(kCategory, "SetEquippedTitle completion missing");
        return ExecCallResult::Success;
    }

    if (!completion.ok)
    {
        FWLOG_WARN(kCategory,
            "SetEquippedTitle failed (sid=%u, dbError=%u)",
            completion.sessionId,
            completion.errorCode);
        if (svc.network != nullptr)
        {
            (void)ServerPacketStager::StageTitleEquipResultPacket(
                *svc.network,
                completion.sessionId,
                completion.clientRequestId,
                false,
                InvalidTitleId,
                kTitleEquipReasonDatabaseError);
        }
        return ExecCallResult::Success;
    }

    SetEquippedTitlePayload payload{};
    if (completion.payloadKey == InvalidDBPayloadKey ||
        !svc.database->ResultStore().TakePayload(
            completion.payloadKey,
            payload))
    {
        FWLOG_WARN(kCategory, "SetEquippedTitle payload missing");
        return ExecCallResult::Success;
    }

    const SessionFlow* const flow =
        svc.sessionFlow != nullptr
        ? svc.sessionFlow->FindFlow(completion.sessionId)
        : nullptr;
    if (flow == nullptr || flow->accountId != payload.accountId)
    {
        FWLOG_WARN(kCategory,
            "SetEquippedTitle stale completion dropped (sid=%u)",
            completion.sessionId);
        return ExecCallResult::Success;
    }

    const bool success =
        payload.resultCode == kTitleEquipReasonSuccess;
    if (svc.network != nullptr)
    {
        (void)ServerPacketStager::StageTitleEquipResultPacket(
            *svc.network,
            completion.sessionId,
            completion.clientRequestId,
            success,
            payload.equippedTitleId,
            payload.resultCode);
    }

    // 장착에 성공했다면, 같은 월드(Plaza)에 있는 모든 플레이어에게
    // 이 플레이어가 어떤 칭호를 끼고 있는지 브로드캐스트한다.
    // 본인 세션도 currentWorldId 세션 목록에 포함되므로 함께 통지된다.
    if (success &&
        svc.network != nullptr &&
        flow->controlledNetId.IsValid() &&
        flow->currentWorldId.IsValid())
    {
        std::vector<SessionId> worldSessionIds;
        svc.sessionFlow->CollectSessionsInWorld(
            flow->currentWorldId,
            worldSessionIds);
        (void)ServerPacketStager::StageTitleReplicationPacketToSessions(
            *svc.network,
            worldSessionIds,
            flow->controlledNetId,
            payload.equippedTitleId);
    }

    return ExecCallResult::Success;
}
