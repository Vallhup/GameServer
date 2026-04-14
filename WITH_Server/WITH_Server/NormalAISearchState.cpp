#include "pch.h"
#include "NormalAISearchState.h"

#include "IAIMovementPolicy.h"

void NormalAISearchState::Enter(AIContext& ctx) const
{
	if (ctx.command)
		ctx.command->ClearAll();
}

void NormalAISearchState::DecisionUpdate(AIContext& ctx, const double decisionDT) const
{
	(void)decisionDT;

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
		return;
	}
}

void NormalAISearchState::FrameUpdate(AIContext& ctx, const double dT) const
{
	(void)dT;

	ctx.movementPolicy->BuildSearchIntent(ctx);
}
