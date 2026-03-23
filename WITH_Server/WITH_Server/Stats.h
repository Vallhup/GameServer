#pragma once

#include "Component.h"

struct BaseVital : Component
{
	int maxHp{ 20 };
	int maxStamina{ 100 };
	double staminaRecoveryPerSec{ 10.0 };
};

struct BaseAttribute : Component
{
	int power{ 10 };
	double attackSpeed{ 1.0 };
	int defense{ 10 };
	double moveSpeed{ 2.0 };
};

struct FinalVital : Component
{
	int maxHp{ 20 };
	int maxStamina{ 100 };
	double staminaRecoveryPerSec{ 10.0 };
	bool dirty{ false };
};

struct FinalAttribute : Component
{
	int power{ 10 };
	double attackSpeed{ 1.0 };
	int defense{ 10 };
	double moveSpeed{ 2.0 };
	bool dirty{ false };
};

struct Vital : Component
{
	int curHp{ 20 };
	int curStamina{ 100 };
};