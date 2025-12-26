#pragma once

#include <array>
#include <variant>

#include "Entity.h"

/* -------- [ Input Event ]-------- */

enum class EventType {
	EV_CONNECT,
	EV_DISCONNECT,
	EV_MOVE,
	EV_ACTION
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
};

struct ActionEvent {
	int sessionId;
	bool isAttacking;
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

struct OutputEvent {
	Entity entity;
	DirtyType type;
};