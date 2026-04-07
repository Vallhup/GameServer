#include "pch.h"
#include "NormalAIChaseState.h"

#include "IAIMovementPolicy.h"

void NormalAIChaseState::Enter(AIContext& ctx) const
{
	if (ctx.command)
		ctx.command->ClearAll();
}

void NormalAIChaseState::DecisionUpdate(AIContext& ctx, const double decisionDT) const
{
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

void NormalAIChaseState::FrameUpdate(AIContext& ctx, const double dT) const
{
	ctx.command->hasLook = true;
	ctx.command->target = ctx.blackboard->currentTarget;

	ctx.movementPolicy->BuildChaseIntent(ctx);
}