#include "pch.h"
#include "AIChaseState.h"

#include "AICombatRangePolicy.h"
#include "IAIMovementPolicy.h"

void AIChaseState::Enter(AIContext& ctx) const
{
	if (ctx.intent)
		ctx.intent->ClearAll();
}

void AIChaseState::DecisionUpdate(AIContext& ctx, const double decisionDT) const
{
	if (ShouldReturnHome(ctx))
	{
		ctx.decision->RequestTransition(AIStateType::ReturnHome);
		return;
	}

	if (ShouldSearchForTarget(ctx))
	{
		ctx.decision->RequestTransition(AIStateType::Search);
		return;
	}

	if (AICombatRangePolicy::ShouldEnterCombat(ctx))
	{
		ctx.decision->RequestTransition(AIStateType::Combat);
		return;
	}
}

void AIChaseState::FrameUpdate(AIContext& ctx, const double dT) const
{
	ctx.intent->hasLook = true;
	ctx.intent->target = ctx.blackboard->currentTarget;

	ctx.movementPolicy->BuildChaseIntent(ctx);
}

bool AIChaseState::ShouldReturnHome(const AIContext& ctx) noexcept
{
	return
		ctx.blackboard != nullptr &&
		ctx.blackboard->returningHome;
}

bool AIChaseState::ShouldSearchForTarget(const AIContext& ctx) noexcept
{
	return
		ctx.perception == nullptr ||
		!ctx.perception->hasTarget;
}
