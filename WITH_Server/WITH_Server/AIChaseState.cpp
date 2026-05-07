#include "pch.h"
#include "AIChaseState.h"

#include "IAIMovementPolicy.h"

void AIChaseState::Enter(AIContext& ctx) const
{
	if (ctx.intent)
		ctx.intent->ClearAll();
}

void AIChaseState::DecisionUpdate(AIContext& ctx, const double decisionDT) const
{
	if (ctx.blackboard->returningHome)
	{
		ctx.decision->RequestTransition(AIStateType::ReturnHome);
		return;
	}

	if (!ctx.perception->hasTarget)
	{
		ctx.decision->RequestTransition(AIStateType::Search);
		return;
	}

	if (ctx.perception->targetInAttackRange && ctx.perception->targetInFront)
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
