#pragma once

#include "ExecutionContextTypes.h"
#include "ExecutionCoreTypes.h"

// IOCP DynamicTask ExecFn 패킷 핸들러 선언.
// 각 함수는 PacketHandlerRegistrar 에서 DynamicTaskTypeDesc::dispatchFn 으로 등록된다.
// payloadKey = SendBuffer* (RAII: 핸들러가 SendBufferPtr로 소유권 획득 후 자동 해제)
// scopeId    = SessionId (IocpConnection::OnRecvComplete 제출 시 설정)

ExecCallResult HandleLoginPacket(NodeExecContext& ctx);
ExecCallResult HandleCharacterSelectPacket(NodeExecContext& ctx);
ExecCallResult HandleMovePacket(NodeExecContext& ctx);
ExecCallResult HandleAttackPacket(NodeExecContext& ctx);
ExecCallResult HandleDodgePacket(NodeExecContext& ctx);
ExecCallResult HandleGuardPacket(NodeExecContext& ctx);
ExecCallResult HandleParryPacket(NodeExecContext& ctx);
ExecCallResult HandleUseItemPacket(NodeExecContext& ctx);
ExecCallResult HandleWorldTransitionRequestPacket(NodeExecContext& ctx);
ExecCallResult HandleWorldTransitionReadyPacket(NodeExecContext& ctx);

// 연결 종료 이벤트 핸들러 — payloadKey == 0 (SendBuffer 없음)
ExecCallResult HandleDisconnectedEvent(NodeExecContext& ctx);
