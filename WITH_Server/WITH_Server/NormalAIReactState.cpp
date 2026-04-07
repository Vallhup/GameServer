#include "pch.h"
#include "NormalAIReactState.h"

void NormalAIReactState::Enter(AIContext& ctx) const
{
	ctx.command->ClearAll();

	if (!ctx.reaction->instigator.IsNull())
	{
		ctx.blackboard->lastAttacker = ctx.reaction->instigator;
		ctx.blackboard->forceRetarget = true;
	}
}

void NormalAIReactState::DecisionUpdate(AIContext& ctx, const double decisionDT) const
{
	if (ctx.decision->stateTime >= 0.1)
	{
		if (ctx.perception->hasTarget)
			ctx.decision->RequestTransition(AIStateType::Chase);

		else
			ctx.decision->RequestTransition(AIStateType::Idle);
	}
}

void NormalAIReactState::FrameUpdate(AIContext& ctx, const double dT) const
{
}