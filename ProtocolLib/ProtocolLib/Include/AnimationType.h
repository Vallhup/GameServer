#pragma once

#include "types.h"

enum class AnimationType : uint8 {
	None,
	Knight_Idle,
	Knight_Walk,
	Knight_Run,
	Knight_Attack,
	Knight_Dead,
	Knight_Drinking,
	Knight_Guard,
	Knight_Hit,
	Knight_Parry,
	Knight_Dodge,
	Knight_Stun,
};

inline uint32 ToInt(AnimationType type) { return static_cast<uint32>(type); }

namespace std {
	template<>
	struct hash<AnimationType> {
		size_t operator()(const AnimationType& type) const noexcept
		{
			return std::hash<uint32>()(ToInt(type));
		}
	};
}