#include "pch.h"
#include "AICombatRangePolicy.h"

#include "AIBehaviorDef.h"
#include "IAIState.h"

bool AICombatRangePolicy::ShouldEnterCombat(const AIContext& ctx) noexcept
{
	return
		ctx.perception != nullptr &&
		ctx.perception->hasTarget &&
		ctx.perception->distanceToTarget <= ResolveCombatEnterRange(ctx) &&
		ctx.perception->targetInFront;
}

bool AICombatRangePolicy::ShouldExitCombat(const AIContext& ctx) noexcept
{
	return
		ctx.perception == nullptr ||
		ctx.perception->distanceToTarget > ResolveCombatExitRange(ctx);
}

bool AICombatRangePolicy::ShouldHoldChasePosition(
	const AIContext& ctx) noexcept
{
	return
		ctx.perception != nullptr &&
		ctx.perception->hasTarget &&
		ctx.perception->distanceToTarget <= ResolveCombatEnterRange(ctx);
}

double AICombatRangePolicy::ResolveCombatEnterRange(
	const AIContext& ctx) noexcept
{
	return ctx.perceptionTuning != nullptr
		? std::max(0.0, ctx.perceptionTuning->attackRange)
		: 0.0;
}

double AICombatRangePolicy::ResolveCombatExitRange(
	const AIContext& ctx) noexcept
{
	if (ctx.perceptionTuning == nullptr)
		return 0.0;

	return
		std::max(0.0, ctx.perceptionTuning->attackRange) +
		std::max(0.0, ctx.perceptionTuning->combatExitRangeBonus);
}
