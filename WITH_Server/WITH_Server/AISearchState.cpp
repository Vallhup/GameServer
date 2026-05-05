#include "pch.h"
#include "AISearchState.h"

#include "AIBehaviorDef.h"
#include "IAIMovementPolicy.h"

void AISearchState::Enter(AIContext& ctx) const
{
	if (ctx.command)
		ctx.command->ClearAll();
}

void AISearchState::DecisionUpdate(AIContext& ctx, const double decisionDT) const
{
	if (ctx.blackboard->returningHome)
	{
		ctx.decision->RequestTransition(AIStateType::ReturnHome);
		return;
	}

	if (ctx.perception->hasTarget)
	{
		ctx.decision->RequestTransition(AIStateType::Chase);
		return;
	}

	if (ctx.blackboard->timeSinceCurrentTargetSeen >
		ctx.perceptionTuning->loseSightGraceTime)
	{
		ctx.blackboard->returningHome = true;
		ctx.blackboard->returnHomeLockoutAcc = 0.0;
		ctx.decision->RequestTransition(AIStateType::ReturnHome);
	}
}

void AISearchState::FrameUpdate(AIContext& ctx, const double dT) const
{
	ctx.movementPolicy->BuildSearchIntent(ctx);
}
