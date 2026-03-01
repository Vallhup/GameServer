#pragma once

#include "Component.h"
#include "BuffData.h"

// 추후에 BuffsComp에 통합할 예정
struct ParryBuf : Component {
	int remaining{ 0 };
	// TEMP : Parry 성공 시 추가 데미지
	double additionalDamage{ 1.0f };
};

struct BuffsComp : Component
{
	std::vector<BuffInstance> buffs;
};