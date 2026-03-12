#pragma once

#include <variant>

#include "Entity.h"
#include "AnimationType.h"

/* -------- [ Input Event ]-------- */

enum class EventType {
	EV_CONNECT,
	EV_DISCONNECT,
	EV_MOVE,
	EV_ACTION
};

enum class ActionRequestType {
	Attack,
	Dodge,
	Parry,
	Guard,
};

namespace std {
	template<>
	struct hash<EventType> {
		size_t operator()(const EventType& id) const noexcept
		{
			return std::hash<int>()(static_cast<int>(id));
		}
	};
}

struct ConnectEvent {
	int sessionId;
};

struct DisconnectEvent 
{
	uint32_t connId;
	NetId netId;
	Entity entity;
};

struct MoveEvent {
	NetId id;
	int inputX;
	int inputZ;
	float yaw;
	bool isRun;
};

struct ActionEvent {
	NetId id;
	ActionRequestType type;
	float dirX;
	float dirZ;
	bool input;
};

using EventPayload = std::variant<
	ConnectEvent,
	DisconnectEvent,
	MoveEvent,
	ActionEvent
>;

struct Event {
	EventType type;
	EventPayload payload;
};

/* -------- [ Lifecycle Event ]-------- */

enum class LifecycleEventType : uint8_t
{
	None,
	Spawned,
	Despawned,
};

struct LifecycleEventPayload
{
	union 
	{
		struct
		{
			uint32_t connId;
			Entity entity;
		} despawn;

		uint32 raw{ 0 };
	};
};

struct LifecycleEvent {
	LifecycleEventType type;
	NetId netId;
	Entity entity;
	uint32_t connId{ std::numeric_limits<uint32_t>::max() };

	inline static LifecycleEvent Spawned(NetId netId)
	{
		LifecycleEvent ev
		{
			.type = LifecycleEventType::Spawned,
			.netId = netId,
		};
		return ev;
	}

	inline static LifecycleEvent Despawned(NetId netId, uint32_t connId, Entity entity)
	{
		LifecycleEvent ev
		{ 
			.type = LifecycleEventType::Despawned,
			.netId = netId,
			.entity = entity,
			.connId = connId
		};
		return ev;
	}
};

/* -------- [ Game Event ]-------- */

enum class CollisionType : uint8 { Strike, Clash };

struct CombatCollisionEvent 
{
	CollisionType type;

	Entity attacker;
	Entity victim;

	uint32 attackId;
	uint16 aIndex;
	uint16 bIndex;
};

enum class ActionRequestReason : uint8 {
	None,
	FromCombat,
	FromInput,
	FromAI
};

struct ActionRequestEvent {
	Entity entity;
	ActionType actionType;
	AttackType attackType;
	ActionRequestReason reason;
};

struct DeathEvent
{
	Entity dead;
	Entity killer;
	EntityType deadType{ EntityType::None };
};