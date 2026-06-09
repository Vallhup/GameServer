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
ExecCallResult HandleTimeSyncPacket(NodeExecContext& ctx);
ExecCallResult HandleCharacterSelectPacket(NodeExecContext& ctx);
ExecCallResult HandleMovePacket(NodeExecContext& ctx);
ExecCallResult HandleAttackPacket(NodeExecContext& ctx);
ExecCallResult HandleDodgePacket(NodeExecContext& ctx);
ExecCallResult HandleGuardPacket(NodeExecContext& ctx);
ExecCallResult HandleParryPacket(NodeExecContext& ctx);
ExecCallResult HandleUseItemPacket(NodeExecContext& ctx);
ExecCallResult HandleWorldTransitionRequestPacket(NodeExecContext& ctx);
ExecCallResult HandleWorldTransitionReadyPacket(NodeExecContext& ctx);
ExecCallResult HandleFinalClearChoiceSubmitPacket(NodeExecContext& ctx);
ExecCallResult HandlePartyUiOpenedPacket(NodeExecContext& ctx);
ExecCallResult HandlePartyUiClosedPacket(NodeExecContext& ctx);
ExecCallResult HandlePartyListRefreshPacket(NodeExecContext& ctx);
ExecCallResult HandlePartyCreatePacket(NodeExecContext& ctx);
ExecCallResult HandlePartyJoinRequestPacket(NodeExecContext& ctx);
ExecCallResult HandlePartyJoinAcceptPacket(NodeExecContext& ctx);
ExecCallResult HandlePartyJoinRejectPacket(NodeExecContext& ctx);
ExecCallResult HandleRespawnRequestPacket(NodeExecContext& ctx);
ExecCallResult HandleStatUiOpenedPacket(NodeExecContext& ctx);
ExecCallResult HandleTitleEquipRequestPacket(NodeExecContext& ctx);
ExecCallResult HandleDisconnectedEvent(NodeExecContext& ctx);

// ---------------------------------------------------------------------------
// 전투 통계 / 칭호 DB 완료 핸들러
// ---------------------------------------------------------------------------
ExecCallResult HandleIncrementMonsterKillCountResult(NodeExecContext& ctx);
ExecCallResult HandleIncrementDeathByMonsterCountResult(NodeExecContext& ctx);
ExecCallResult HandleUnlockTitleResult(NodeExecContext& ctx);
ExecCallResult HandleGetAccountTitlesResult(NodeExecContext& ctx);
ExecCallResult HandleSetEquippedTitleResult(NodeExecContext& ctx);
