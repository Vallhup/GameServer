#pragma once

#include "DynamicTaskTypes.h"
#include "ExecutionContextTypes.h"
#include "ExecutionCoreTypes.h"

class DynamicTaskTypeRegistry;
class ExecutionSourceRegistry;
class NetworkRuntime;

void RegisterServerPacketHandlers(
	DynamicTaskTypeRegistry& taskRegistry,
	ExecutionSourceRegistry& sourceRegistry,
	NetworkRuntime& network,
	DynamicTaskTypeId& outDisconnectedTypeId);

ExecCallResult HandleLoginPacket(NodeExecContext& ctx);
ExecCallResult HandleLoginAuthResult(NodeExecContext& ctx);
ExecCallResult HandleRegisterAccountResult(NodeExecContext& ctx);
ExecCallResult HandleCharacterSelectPacket(NodeExecContext& ctx);
ExecCallResult HandleMovePacket(NodeExecContext& ctx);
ExecCallResult HandleAttackPacket(NodeExecContext& ctx);
ExecCallResult HandleDodgePacket(NodeExecContext& ctx);
ExecCallResult HandleGuardPacket(NodeExecContext& ctx);
ExecCallResult HandleParryPacket(NodeExecContext& ctx);
ExecCallResult HandleUseItemPacket(NodeExecContext& ctx);
ExecCallResult HandleWorldTransitionRequestPacket(NodeExecContext& ctx);
ExecCallResult HandleWorldTransitionReadyPacket(NodeExecContext& ctx);
ExecCallResult HandlePartyUiOpenedPacket(NodeExecContext& ctx);
ExecCallResult HandlePartyListRefreshPacket(NodeExecContext& ctx);
ExecCallResult HandlePartyCreatePacket(NodeExecContext& ctx);
ExecCallResult HandlePartyJoinRequestPacket(NodeExecContext& ctx);
ExecCallResult HandlePartyJoinAcceptPacket(NodeExecContext& ctx);
ExecCallResult HandlePartyJoinRejectPacket(NodeExecContext& ctx);
ExecCallResult HandleDisconnectedEvent(NodeExecContext& ctx);
