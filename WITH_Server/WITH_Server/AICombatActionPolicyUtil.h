#pragma once

#include "AIBehaviorDef.h"

struct AIContext;
struct CombatActionSelection;

namespace AICombatActionPolicyUtil
{
	int PseudoRand(uint64_t entityId, uint32_t sequence, int range);

	void FillAttackDirectionTowardTarget(
		const AIContext& ctx,
		CombatActionSelection& out);
}
