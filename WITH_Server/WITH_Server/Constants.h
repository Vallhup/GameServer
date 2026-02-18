#pragma once

#include <cstddef>

#include "EntityType.h"

inline constexpr size_t max_threads = 16;
inline constexpr short BUFFER_SIZE = 4096 * 4;

enum class AttackType : uint8 {
	None,

	// Player
	Light,
	Heavy,

	// Boss
	JumpSlash,
	MultiSlash,
	DashSlash,
	Thrust,
	Slash,
	Meteor,

	Count
};

constexpr uint32 ToInt(AttackType type) { return static_cast<uint32>(type); }

enum class ActionType : uint8 {
	None,
	Attack,
	Dodge,
	Parry,
	Stun,
	Hit,
	Guard,
	Dead,
	Count
};

namespace std {
	template<>
	struct hash<ActionType> {
		size_t operator()(const ActionType& id) const noexcept
		{
			return std::hash<uint8>()(static_cast<uint8>(id));
		}
	};
}

constexpr size_t ToIndex(ActionType type) { return static_cast<size_t>(type); }
constexpr uint32 Bit(ActionType type) { return (uint32)1u << ToIndex(type); }

static constexpr size_t actionCnt{ static_cast<size_t>(ActionType::Count) };
static constexpr size_t entityCnt{ static_cast<size_t>(EntityType::Count) };
static constexpr size_t attackCnt{ static_cast<size_t>(AttackType::Count) };

static constexpr size_t Index(ActionType action, EntityType entity, AttackType attack)
{
	return (static_cast<size_t>(action) * entityCnt + static_cast<size_t>(entity)) * attackCnt + static_cast<size_t>(attack);
}