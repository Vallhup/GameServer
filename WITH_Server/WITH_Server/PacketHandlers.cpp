#include "pch.h"
#include "PacketHandlers.h"

#include "CharacterDataService.h"
#include "CharacterSpawnService.h"
#include "DynamicTaskTypes.h"
#include "ECS/System/Phase2/ResolveAbilityStateSystem.h"
#include "ECS/System/Phase2/ResolveLocomotionStateSystem.h"
#include "ECS/System/Phase2/ResolvePlayerNetworkCompensationSystem.h"
#include "ExecutionContextTypes.h"
#include "ExecutionCoreTypes.h"
#include "ExecutionOps.h"
#include "FrameworkRuntime.h"
#include "FrameworkLog.h"
#include "LoginAuth.h"
#include "RegisterAccount.h"
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

    DynamicTaskTypeId g_loginAuthResultTaskTypeId{ InvalidDynamicTaskTypeId };
    DynamicTaskTypeId g_registerAccountResultTaskTypeId{ InvalidDynamicTaskTypeId };

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
            return nullptr;

        if (ctx.TryGetWorldId() != worldId)
            return nullptr;

        return ctx.TryGetRuntime();
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
            return false;
        }

        Entity entity = Entity::Null();
        if (!ops->TryResolveEntity(worldId, netId, entity) ||
            entity.IsNull())
        {
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
            return false;
        }

        if (identity->ownerSessionId != sessionId ||
            identity->netId != netId)
        {
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

    DynamicTaskTypeId RegisterPacketDynamicTask(
        DynamicTaskTypeRegistry& taskRegistry,
        ExecutionSourceRegistry& sourceRegistry,
        NetworkRuntime& network,
        PacketType packetType,
        ExecFn dispatchFn,
        const char* debugName,
        DynamicTaskTargetKind targetKind = DynamicTaskTargetKind::ExplicitScope,
        bool gameplayInput = false)
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

        const DynamicTaskTypeId typeId =
            taskRegistry.Register(desc, sourceRegistry);
        if (typeId != InvalidDynamicTaskTypeId)
        {
            network.RegisterPacketHandler(
                static_cast<uint16_t>(packetType),
                typeId);
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
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_TIME_SYNC,                &HandleTimeSyncPacket,              "Pkt_CS_TIME_SYNC");
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
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_PARTY_UI_OPENED,          &HandlePartyUiOpenedPacket,          "Pkt_CS_PARTY_UI_OPENED");
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_PARTY_UI_CLOSED,          &HandlePartyUiClosedPacket,          "Pkt_CS_PARTY_UI_CLOSED");
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_PARTY_LIST_REFRESH,       &HandlePartyListRefreshPacket,       "Pkt_CS_PARTY_LIST_REFRESH");
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_PARTY_CREATE,             &HandlePartyCreatePacket,            "Pkt_CS_PARTY_CREATE");
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_PARTY_JOIN_REQUEST,       &HandlePartyJoinRequestPacket,       "Pkt_CS_PARTY_JOIN_REQUEST");
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_PARTY_JOIN_ACCEPT,        &HandlePartyJoinAcceptPacket,        "Pkt_CS_PARTY_JOIN_ACCEPT");
    RegisterPacketDynamicTask(taskRegistry, sourceRegistry, network,
        PacketType::CS_PARTY_JOIN_REJECT,        &HandlePartyJoinRejectPacket,        "Pkt_CS_PARTY_JOIN_REJECT");

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
        return ExecCallResult::Failed;

    Protocol::CS_TIME_SYNC_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    auto& svc = PacketHandlerContext::Get();
    if (svc.networkTiming != nullptr)
    {
        svc.networkTiming->HandleClientEcho(
            ResolveSessionId(ctx),
            pkt.probeseq(),
            pkt.echoedserversendtimems());
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
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    if (svc.sessionFlow == nullptr ||
        ResolveWorldRuntime(ctx, sessionId, *svc.sessionFlow) == nullptr)
    {
        return ExecCallResult::Success;
    }

    Protocol::CS_MOVE_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    if (svc.networkTiming != nullptr)
    {
        svc.networkTiming->RecordPeriodicInputArrival(sessionId);
    }

    PlayerInputTarget target{};
    if (!TryResolvePlayerInputTarget(ctx, sessionId, *svc.sessionFlow, target))
    {
        return ExecCallResult::Success;
    }

    ApplyMoveInput(*target.runtime, pkt, *target.input);
    return ExecCallResult::Success;
}

ExecCallResult HandleAttackPacket(NodeExecContext& ctx)
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

    Protocol::CS_ATTACK_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    PlayerInputTarget target{};
    if (!TryResolvePlayerInputTarget(ctx, sessionId, *svc.sessionFlow, target))
    {
        return ExecCallResult::Success;
    }

    ApplyAttackInput(*target.runtime, pkt, *target.input);
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

ExecCallResult HandlePartyUiOpenedPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    Protocol::CS_PARTY_UI_OPENED_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    SubmitPartyCommand(PartyCommand{
        .kind = PartyCommandKind::UiOpened,
        .actorSessionId = ResolveSessionId(ctx),
        .clientRequestId = pkt.clientrequestid()
    });
    return ExecCallResult::Success;
}

ExecCallResult HandlePartyUiClosedPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    Protocol::CS_PARTY_UI_CLOSED_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    SubmitPartyCommand(PartyCommand{
        .kind = PartyCommandKind::UiClosed,
        .actorSessionId = ResolveSessionId(ctx),
        .clientRequestId = pkt.clientrequestid()
    });
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

    Protocol::CS_PARTY_CREATE_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    SubmitPartyCommand(PartyCommand{
        .kind = PartyCommandKind::CreateParty,
        .actorSessionId = ResolveSessionId(ctx),
        .clientRequestId = pkt.clientrequestid()
    });
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
