#include "pch.h"
#include "PacketHandlerRegistrar.h"

#include "DynamicTaskTypes.h"
#include "ExecutionCoreTypes.h"
#include "ExecutionSourceTypes.h"
#include "NetworkRuntime.h"
#include "PacketHandlers.h"
#include "PacketType.h"

namespace
{
	DynamicTaskTypeId RegisterHandler(
		DynamicTaskTypeRegistry& taskRegistry,
		ExecutionSourceRegistry& sourceRegistry,
		NetworkRuntime& network,
		PacketType packetType,
		ExecFn dispatchFn,
		const char* debugName)
	{
		DynamicTaskTypeDesc desc{};
		desc.debugName    = debugName;
		desc.defaultPhase = ExecPhase::Simulate;
		desc.defaultLane  = ExecLane::Serial;
		desc.dispatchFn   = dispatchFn;

		const DynamicTaskTypeId typeId = taskRegistry.Register(desc, sourceRegistry);
		if (typeId != InvalidDynamicTaskTypeId)
		{
			network.RegisterPacketHandler(
				static_cast<uint16_t>(packetType), typeId);
		}
		return typeId;
	}
}

void PacketHandlerRegistrar::Register(
	DynamicTaskTypeRegistry& taskRegistry,
	ExecutionSourceRegistry& sourceRegistry,
	NetworkRuntime& network,
	DynamicTaskTypeId& outDisconnectedTypeId)
{
	RegisterHandler(taskRegistry, sourceRegistry, network,
		PacketType::CS_LOGIN,                  &HandleLoginPacket,                  "Pkt_CS_LOGIN");
	RegisterHandler(taskRegistry, sourceRegistry, network,
		PacketType::CS_CHARACTER_SELECT,       &HandleCharacterSelectPacket,        "Pkt_CS_CHARACTER_SELECT");
	RegisterHandler(taskRegistry, sourceRegistry, network,
		PacketType::CS_MOVE,                   &HandleMovePacket,                   "Pkt_CS_MOVE");
	RegisterHandler(taskRegistry, sourceRegistry, network,
		PacketType::CS_ATTACK,                 &HandleAttackPacket,                 "Pkt_CS_ATTACK");
	RegisterHandler(taskRegistry, sourceRegistry, network,
		PacketType::CS_DODGE,                  &HandleDodgePacket,                  "Pkt_CS_DODGE");
	RegisterHandler(taskRegistry, sourceRegistry, network,
		PacketType::CS_GUARD,                  &HandleGuardPacket,                  "Pkt_CS_GUARD");
	RegisterHandler(taskRegistry, sourceRegistry, network,
		PacketType::CS_PARRY,                  &HandleParryPacket,                  "Pkt_CS_PARRY");
	RegisterHandler(taskRegistry, sourceRegistry, network,
		PacketType::CS_WORLD_TRANSITION_REQUEST, &HandleWorldTransitionRequestPacket, "Pkt_CS_WORLD_TRANSITION_REQUEST");
	RegisterHandler(taskRegistry, sourceRegistry, network,
		PacketType::CS_WORLD_TRANSITION_READY, &HandleWorldTransitionReadyPacket,   "Pkt_CS_WORLD_TRANSITION_READY");

	// Disconnect 이벤트 타입 — 패킷이 없어 PacketDispatchTable에는 등록하지 않는다
	{
		DynamicTaskTypeDesc desc{};
		desc.debugName    = "Evt_Disconnected";
		desc.defaultPhase = ExecPhase::Simulate;
		desc.defaultLane  = ExecLane::Serial;
		desc.dispatchFn   = &HandleDisconnectedEvent;

		outDisconnectedTypeId = taskRegistry.Register(desc, sourceRegistry);
	}
}
