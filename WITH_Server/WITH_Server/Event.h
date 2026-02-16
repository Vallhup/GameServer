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
	GuardRelease
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
	int sessionId;
};

struct MoveEvent {
	int sessionId;
	int inputX;
	int inputZ;
	float yaw;
	bool isRun;
};

struct ActionEvent {
	int sessionId;
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

		uint32 raw{ 0 };
	};
};

struct OutputEvent {
	Entity entity;
	DirtyType type;
	OutputEventPayload payload;

	static OutputEvent AnimationChanged(Entity e, AnimationType curr)
	{
		OutputEvent ev{ e, DirtyType::AnimationChanged, { } };
		ev.payload.anim = { curr };
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
