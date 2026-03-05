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

struct DisconnectEvent {
	NetId id;
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

/* -------- [ Output Event ]-------- */

enum class DirtyType {
	Spawned,
	Despawned,
	Moved,
	AnimationChanged,
	StatsChanged
};

struct OutputEventPayload
{
	union {
		struct {
			AnimationType currType;
		} anim;

		struct
		{
			int curHp;
			int curStamina;
		} stat;

		uint32 raw{ 0 };
	};
};

struct OutputEvent {
	NetId netId;
	DirtyType type;
	OutputEventPayload payload;

	static OutputEvent AnimationChanged(NetId netId, AnimationType curr)
	{
		OutputEvent ev{ netId, DirtyType::AnimationChanged, { } };
		ev.payload.anim = { curr };
		return ev;
	}

	static OutputEvent StatChanged(NetId netId, int curHp, int curStamina)
	{
		OutputEvent ev{ netId, DirtyType::StatsChanged, { } };
		ev.payload.stat = { curHp, curStamina };
		return ev;
	}
};

/* -------- [ Game Event ]-------- */

enum class CollisionType : uint8 { Strike, Clash };

struct CombatCollisionEvent {
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