#pragma once

#include <cstddef>
#include <limits>

#include "GameplayContentIds.h"
#include "Entity.h"

struct AIContext;

struct CombatActionSelection
{
	static constexpr size_t InvalidActionIndex =
		std::numeric_limits<size_t>::max();

	bool shouldAttack{ false };
	size_t selectedActionIndex{ InvalidActionIndex };
	AbilityId selectedAbilityId{ InvalidAbilityId };
	Entity target{ Entity::Null() };
	float directionX{ 0.0f };
	float directionZ{ 0.0f };
};

class IAICombatActionPolicy {
public:
	virtual ~IAICombatActionPolicy() = default;

	virtual CombatActionSelection SelectAction(const AIContext& ctx) const = 0;
	virtual void CommitSelection(
		AIContext& ctx,
		const CombatActionSelection& selection) const = 0;
};
