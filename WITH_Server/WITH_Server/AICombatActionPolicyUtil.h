#pragma once

#include "AIBehaviorDef.h"

#include <span>

struct AIContext;
struct CombatActionSelection;

namespace AICombatActionPolicyUtil
{
	int PseudoRand(uint64_t entityId, uint32_t sequence, int range);

	void FillAttackDirectionTowardTarget(
		const AIContext& ctx,
		CombatActionSelection& out);

	AbilityId PickWeightedAbility(
		std::span<const WeightedActionEntry> actions,
		AbilityId lastUsed,
		int roll,
		uint16_t repeatWeightPercent = 25);
}
