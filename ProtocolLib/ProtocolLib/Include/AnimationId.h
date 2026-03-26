#pragma once

#include "types.h"

enum class AnimationId : uint8 {
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

	Imp_Idle,
	Imp_Walk,
	Imp_melee_1,
	Imp_melee_2,
	Imp_melee_3,
	Imp_melee_4,
	Imp_melee_5,
	Imp_Stun,
	Imp_Hit,
	Imp_Dead,
	
	FinalBoss_Idle,
	FinalBoss_Walk,
	FinalBoss_Thrust,
	FinalBoss_Slash,
	FinalBoss_DashSlash,
	FinalBoss_JumpSlash,
	FinalBoss_MultiSlash,
	FinalBoss_Stun,
	FinalBoss_Hit,
	FinalBoss_Dead,
};

inline uint32 ToInt(AnimationId type) { return static_cast<uint32>(type); }

namespace std {
	template<>
	struct hash<AnimationId> {
		size_t operator()(const AnimationId& type) const noexcept
		{
			return std::hash<uint32>()(ToInt(type));
		}
	};
}