#pragma once

#include "Component.h"

struct ParryBuf : public Component {
	int remaining{ 0 };
	// TEMP : Parry 성공 시 추가 데미지
	double additionalDamage{ 1.0f };
};