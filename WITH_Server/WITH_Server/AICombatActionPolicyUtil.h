#pragma once

#include "AIFSMRegistry.h"

#include <span>

namespace AICombatActionPolicyUtil
{
	int PseudoRand(uint64_t entityId, uint32_t sequence, int range);

	void FillAttackDirectionTowardTarget(
		const AIContext& ctx,
		CombatActionSelection& out);

	ActionId PickWeightedAction(
		std::span<const WeightedActionEntry> actions,
		ActionId lastUsed,
		int roll,
		uint16_t repeatWeightPercent = 25);
}
