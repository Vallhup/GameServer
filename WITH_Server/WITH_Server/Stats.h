#pragma once

#include "Component.h"

struct Vital : public Component
{
	int curHp{ 100 };
	int maxHp{ 100 };

	int curStamina{ 100 };
	int maxStamina{ 100 };

	double staminaRecoveryPerSec{ 10.0 };
};

struct Attribute : public Component
{
	int power{ 10 };
	double attackSpeed{ 1.0 };
	int defense{ 10 };
	int moveSpeed{ 2 };
};
