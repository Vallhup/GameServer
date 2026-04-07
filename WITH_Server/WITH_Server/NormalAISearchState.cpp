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
	if (ctx.perception->hasTarget)
	{
		ctx.decision->RequestTransition(AIStateType::Chase);
		return;
	}

	if (ctx.blackboard->timeSinceCurrentTargetSeen >
		ctx.perceptionTuning->loseSightGraceTime)
	{
		ctx.decision->RequestTransition(AIStateType::Idle);
		return;
	}
}

void NormalAISearchState::FrameUpdate(AIContext& ctx, const double dT) const
{
	ctx.movementPolicy->BuildSearchIntent(ctx);
}