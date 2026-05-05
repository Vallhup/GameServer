#pragma once

#include "GameplayContentIds.h"

struct AIContext;

struct CombatActionSelection
{
	bool shouldAttack{ false };
	AbilityId selectedAbilityId{ InvalidAbilityId };
	float directionX{ 0.0f };
	float directionZ{ 0.0f };
};

class IAICombatActionPolicy {
public:
	virtual ~IAICombatActionPolicy() = default;

	virtual CombatActionSelection SelectAction(const AIContext& ctx) const = 0;
};
