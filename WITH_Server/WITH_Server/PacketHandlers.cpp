#include "pch.h"
#include "PacketHandlers.h"

#include "DynamicTaskTypes.h"
#include "ExecutionContextTypes.h"
#include "ExecutionCoreTypes.h"
#include "FrameworkLog.h"
#include "NetworkRuntime.h"
#include "PacketFactory.h"
#include "PacketHandlerContext.h"
#include "PlayerCommand.h"
#include "PlayerEntryService.h"
#include "SessionBindingRegistry.h"
#include "ServerPacketStager.h"
#include "WorldRuntime.h"
#include "IWorldTransitionRequestSink.h"

#include "Protocol.pb.h"
#include "SendBuffer.h"

namespace
{
    constexpr const char* kLogCategory = "PacketHandlers";
    constexpr uint32_t kDefaultCommandSequence = 0;
    constexpr uint32_t kWorldTransitionRejectReasonServerUnavailable = 1;
    constexpr uint32_t kWorldTransitionRejectReasonRequestRejected = 2;

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
    ExecScopeId ResolveWorldScopeId(
        const FrameExecContext& frame,
        WorldId worldId) noexcept
    {
        for (ExecScopeId i = 0;
             i < static_cast<ExecScopeId>(frame.worldIdByScope.size()); ++i)
        {
            if (frame.worldIdByScope[i] == worldId)
                return i;
        }
        return InvalidExecScopeId;
    }

    // sessionId → WorldRuntime* (nullptr이면 미입장)
    WorldRuntime* ResolveWorldRuntime(
        const NodeExecContext& ctx,
        SessionId sessionId,
        const SessionBindingRegistry& sessionBindings) noexcept
    {
        const WorldId worldId =
            sessionBindings.FindCurrentWorldId(sessionId);
        if (!worldId.IsValid())
            return nullptr;

        const ExecScopeId worldScopeId =
            ResolveWorldScopeId(*ctx.frame, worldId);
        if (worldScopeId == InvalidExecScopeId)
            return nullptr;

        return ctx.frame->TryGetRuntime(worldScopeId);
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
        const SessionBindingRegistry& sessionBindings,
        const PlayerEntryService& playerEntryService) noexcept
    {
        if (sessionBindings.HasBinding(sessionId))
            return true;
        if (playerEntryService.FindContext(sessionId) != nullptr)
            return true;
        return false;
    }
}

ExecCallResult HandleLoginPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    if (IsLoginDuplicate(sessionId,
                         *svc.sessionBindings,
                         *svc.playerEntryService))
    {
        FWLOG_WARN(kLogCategory, "Duplicate login rejected (sid=%u)", sessionId);
        (void)svc.network->RequestClose(sessionId, SessionCloseReason::ProtocolError);
        return ExecCallResult::Success;
    }

    if (!svc.playerEntryService->BeginAuthenticatedEntry(sessionId))
    {
        FWLOG_WARN(kLogCategory, "BeginAuthenticatedEntry failed (sid=%u)", sessionId);
        (void)svc.network->RequestClose(sessionId, SessionCloseReason::ProtocolError);
        return ExecCallResult::Success;
    }

    (void)svc.network->RequestCompleteLogin(sessionId);
    return ExecCallResult::Success;
}

ExecCallResult HandleMovePacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    WorldRuntime* world =
        ResolveWorldRuntime(ctx, sessionId, *svc.sessionBindings);
    if (!world)
        return ExecCallResult::Success;

    Protocol::CS_MOVE_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    const NetId netId =
        svc.sessionBindings->FindControlledNetId(sessionId);
    if (!netId.IsValid())
        return ExecCallResult::Success;

    const PlayerMoveCommandPayload payload{
        .inputX = static_cast<float>(pkt.inputx()),
        .inputZ = static_cast<float>(pkt.inputz()),
        .yaw    = pkt.yaw(),
        .isRun  = pkt.isrun() ? uint8_t{ 1 } : uint8_t{ 0 }
    };
    (void)world->EnqueueWorldCommand(
        MakePlayerMoveCommand(sessionId, netId, kDefaultCommandSequence, payload));
    return ExecCallResult::Success;
}

ExecCallResult HandleAttackPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    WorldRuntime* world =
        ResolveWorldRuntime(ctx, sessionId, *svc.sessionBindings);
    if (!world)
        return ExecCallResult::Success;

    Protocol::CS_ATTACK_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    const NetId netId =
        svc.sessionBindings->FindControlledNetId(sessionId);
    if (!netId.IsValid())
        return ExecCallResult::Success;

    const PlayerDirectionCommandPayload payload{
        .dirX = pkt.dirx(),
        .dirZ = pkt.dirz()
    };
    (void)world->EnqueueWorldCommand(
        MakePlayerLightAttackCommand(sessionId, netId, kDefaultCommandSequence, payload));
    return ExecCallResult::Success;
}

ExecCallResult HandleDodgePacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    WorldRuntime* world =
        ResolveWorldRuntime(ctx, sessionId, *svc.sessionBindings);
    if (!world)
        return ExecCallResult::Success;

    Protocol::CS_DODGE_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    const NetId netId =
        svc.sessionBindings->FindControlledNetId(sessionId);
    if (!netId.IsValid())
        return ExecCallResult::Success;

    const PlayerDirectionCommandPayload payload{
        .dirX = pkt.dirx(),
        .dirZ = pkt.dirz()
    };
    (void)world->EnqueueWorldCommand(
        MakePlayerDodgeCommand(sessionId, netId, kDefaultCommandSequence, payload));
    return ExecCallResult::Success;
}

ExecCallResult HandleGuardPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    WorldRuntime* world =
        ResolveWorldRuntime(ctx, sessionId, *svc.sessionBindings);
    if (!world)
        return ExecCallResult::Success;

    Protocol::CS_GUARD_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    const NetId netId =
        svc.sessionBindings->FindControlledNetId(sessionId);
    if (!netId.IsValid())
        return ExecCallResult::Success;

    const PlayerGuardCommandPayload payload{
        .pressed = pkt.input() ? uint8_t{ 1 } : uint8_t{ 0 }
    };
    (void)world->EnqueueWorldCommand(
        MakePlayerGuardCommand(sessionId, netId, kDefaultCommandSequence, payload));
    return ExecCallResult::Success;
}

ExecCallResult HandleParryPacket(NodeExecContext& ctx)
{
    auto buf = AcquirePayload(ctx);
    if (!buf)
        return ExecCallResult::Failed;

    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    WorldRuntime* world =
        ResolveWorldRuntime(ctx, sessionId, *svc.sessionBindings);
    if (!world)
        return ExecCallResult::Success;

    Protocol::CS_PARRY_PACKET pkt{};
    if (!ParseProto(*buf, pkt))
        return ExecCallResult::Success;

    const NetId netId =
        svc.sessionBindings->FindControlledNetId(sessionId);
    if (!netId.IsValid())
        return ExecCallResult::Success;

    const PlayerDirectionCommandPayload payload{
        .dirX = pkt.dirx(),
        .dirZ = pkt.dirz()
    };
    (void)world->EnqueueWorldCommand(
        MakePlayerParryCommand(sessionId, netId, kDefaultCommandSequence, payload));
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

    (void)svc.worldTransitionSink->MarkClientWorldTransitionReady(
        sessionId,
        static_cast<TransferId>(pkt.transferid()));
    return ExecCallResult::Success;
}

ExecCallResult HandleDisconnectedEvent(NodeExecContext& ctx)
{
    auto& svc = PacketHandlerContext::Get();
    const SessionId sessionId = ResolveSessionId(ctx);

    if (svc.sessionBindings != nullptr)
        (void)svc.sessionBindings->Unbind(sessionId);

    if (svc.playerEntryService != nullptr)
        (void)svc.playerEntryService->CancelEntry(sessionId);

    return ExecCallResult::Success;
}
