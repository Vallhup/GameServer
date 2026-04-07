#pragma once

#include "types.h"

enum class AnimationId : uint8 {
	None,

	Knight_Idle,
	Knight_Walk,
	Knight_Run,
	Knight_LightAttack1,
	//Knight_LightAttack2,
	//Knight_LightAttack3,
	//Knight_HeavyAttack,
	//Knight_SpecialAttack,
	Knight_Dodge,
	Knight_Parry,
	Knight_Stun,
	Knight_Hit,
	Knight_Guard,
	Knight_Drinking,
	Knight_Death,


	FinalBoss_Idle,
	FinalBoss_Walk,
	FinalBoss_Thrust,
	FinalBoss_Slash,
	FinalBoss_DashSlash,
	FinalBoss_JumpSlash,
	FinalBoss_MultiSlash,
	FinalBoss_Stun,
	FinalBoss_Hit,
	FinalBoss_Death,

	Imp_Idle_1,
	Imp_Idle_2,
	Imp_Idle_3,
	Imp_Idle_4,
	Imp_Idle_5,
	Imp_Idle_6,
	Imp_Idle_Battlecry,
	Imp_Idle_Roaring,
	Imp_Melee_1,
	Imp_Melee_2,
	Imp_Melee_3,
	Imp_Melee_4,
	Imp_Melee_5,
	Imp_Walk_Back,
	Imp_Walk_Forward,
	Imp_Walk_Left,
	Imp_Walk_Right,
	Imp_Jump_1,
	Imp_Stun,
	Imp_React_Front,
	Imp_React_Left,
	Imp_React_Right,
	Imp_Death_1,
	Imp_Death_2,
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