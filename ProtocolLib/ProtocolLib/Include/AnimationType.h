#pragma once

#include "types.h"

enum class AnimationType : uint8 {
	None,
	Knight_Idle,
	Knight_Walk,
	Knight_Run,
	Knight_Attack,
	Knight_Dodge,
	Knight_Parry,
	Knight_Stun,
	Knight_Hit,
	Knight_Guard,
	Knight_Drinking,
	Knight_Dead,
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