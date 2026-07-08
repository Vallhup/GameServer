#include "pch.h"
#include "AICombatRangePolicy.h"

#include "AICombatActionContextBuilder.h"
#include "AICombatActionSelector.h"
#include "AIBehaviorDef.h"
#include "IAIState.h"

namespace
{
	constexpr double kCombatEnterRangeBufferRatio = 0.05;
	constexpr double kCombatEnterRangeMaxBuffer = 0.35;
}

bool AICombatRangePolicy::ShouldEnterCombat(const AIContext& ctx)
{
	return
		ctx.perception != nullptr &&
		ctx.perception->hasTarget &&
		ctx.perception->distanceToTarget <= ResolveCombatEnterRange(ctx) &&
		ctx.perception->targetInFront &&
		CanSelectCombatAction(ctx);
}

bool AICombatRangePolicy::ShouldExitCombat(const AIContext& ctx) noexcept
{
	return
		ctx.perception == nullptr ||
		ctx.perception->distanceToTarget > ResolveCombatExitRange(ctx);
}

bool AICombatRangePolicy::ShouldHoldChasePosition(
	const AIContext& ctx)
{
	return
		ctx.perception != nullptr &&
		ctx.perception->hasTarget &&
		ctx.perception->distanceToTarget <= ResolveCombatEnterRange(ctx) &&
		CanSelectCombatAction(ctx);
}

bool AICombatRangePolicy::CanSelectCombatAction(const AIContext& ctx)
{
	if (!AICombatActionContextBuilder::CanBuild(ctx))
		return false;

	const AICombatActionSelectionContext selectionContext =
		AICombatActionContextBuilder::Build(ctx);
	const AICombatActionChoice choice =
		AICombatActionSelector::Select(selectionContext);
	return choice.IsValid(selectionContext.actions.size());
}

double AICombatRangePolicy::ResolveCombatEnterRange(
	const AIContext& ctx) noexcept
{
	const double attackRange = ctx.perceptionTuning != nullptr
		? std::max(0.0, ctx.perceptionTuning->attackRange)
		: 0.0;
	const double buffer = std::min(
		kCombatEnterRangeMaxBuffer,
		attackRange * kCombatEnterRangeBufferRatio);
	return std::max(0.0, attackRange - buffer);
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
