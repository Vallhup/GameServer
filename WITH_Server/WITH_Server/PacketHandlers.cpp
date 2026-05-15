#include "pch.h"
#include "PacketHandlers.h"

#include "CharacterDataService.h"
#include "CharacterSpawnService.h"
#include "DynamicTaskTypes.h"
#include "ECS/System/Phase2/ResolveAbilityStateSystem.h"
#include "ECS/System/Phase2/ResolveLocomotionStateSystem.h"
#include "ExecutionContextTypes.h"
#include "ExecutionCoreTypes.h"
#include "ExecutionOps.h"
#include "FrameworkRuntime.h"
#include "FrameworkLog.h"
#include "NetworkRuntime.h"
#include "PacketFactory.h"
#include "PacketHandlerContext.h"
#include "PacketType.h"
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

#include <limits>

namespace
{
    constexpr const char* kLogCategory = "PacketHandlers";
    constexpr uint32_t kWorldTransitionRejectReasonServerUnavailable = 1;
    constexpr uint32_t kWorldTransitionRejectReasonRequestRejected = 2;
    constexpr uint32_t kLoginFailReasonMalformedPacket = 1;
    constexpr uint32_t kLoginFailReasonDuplicateLogin = 2;
    constexpr uint32_t kLoginFailReasonEntryStartFailed = 3;
    constexpr uint32_t kLoginFailReasonAuthRejected = 4;

    struct LoginAuthResult
    {
        bool     accepted{ false };
        uint32_t failReason{ kLoginFailReasonAuthRejected };
    };

    struct PlayerInputTarget
    {
        Entity entity{ Entity::Null() };
        PlayerControlIdentityComp* identity{ nullptr };
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

        out.entity = entity;
        out.identity = identity;
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

    LoginAuthResult AuthenticateLoginRequest(
        const Protocol::CS_LOGIN_PACKET& /*packet*/) noexcept
    {
        // TODO: Replace this stub with the DB-backed authentication result.
        // Keep HandleLoginPacket as the state-transition consumer so async DB
        // completion can reuse the same success/fail path later.
        return LoginAuthResult{ .accepted = true, .failReason = 0 };
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

    DynamicTaskTypeDesc desc{};
    desc.debugName    = "Evt_Disconnected";
    desc.defaultPhase = ExecPhase::Simulate;
    desc.defaultLane  = ExecLane::Serial;
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

    const LoginAuthResult authResult = AuthenticateLoginRequest(pkt);
    if (!authResult.accepted)
    {
        FWLOG_WARN(kLogCategory,
            "Login rejected: auth failed (sid=%u, reason=%u)",
            sessionId,
            authResult.failReason);
        (void)ServerPacketStager::StageLoginFail(
            *svc.network,
            sessionId,
            authResult.failReason);
        LoginFailed failedCommand{};
        failedCommand.reason = authResult.failReason;
        (void)svc.sessionFlow->Dispatch(sessionId, failedCommand);
        (void)svc.network->RequestClose(sessionId, SessionCloseReason::ProtocolError);
        return ExecCallResult::Success;
    }

    const NetId controlledNetId = svc.framework->AllocateNetId();
    if (!controlledNetId.IsValid())
    {
        FWLOG_WARN(kLogCategory, "Login rejected: netId allocation failed (sid=%u)", sessionId);
        (void)ServerPacketStager::StageLoginFail(
            *svc.network,
            sessionId,
            kLoginFailReasonEntryStartFailed);
        LoginFailed failedCommand{};
        failedCommand.reason = kLoginFailReasonEntryStartFailed;
        (void)svc.sessionFlow->Dispatch(sessionId, failedCommand);
        (void)svc.network->RequestClose(sessionId, SessionCloseReason::ProtocolError);
        return ExecCallResult::Success;
    }

    LoginSucceeded succeededCommand{};
    succeededCommand.controlledNetId = controlledNetId;
    flowResult = svc.sessionFlow->Dispatch(sessionId, succeededCommand);
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

    ApplyAbilityInput(
        *target.runtime,
        PlayerAbilityInputType::LightAttack,
        pkt.dirx(),
        pkt.dirz(),
        *target.input);
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

ExecCallResult HandleDisconnectedEvent(NodeExecContext& ctx)
{
    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    if (svc.sessionSystem == nullptr)
        return ExecCallResult::Failed;

    svc.sessionSystem->HandleSessionDisconnected(
        sessionId,
        SessionCloseReason::RemoteClosed);
    return ExecCallResult::Success;
}
