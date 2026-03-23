#include "pch.h"
#include "NormalAIChaseState.h"

#include "IMovementPolicy.h"

void NormalAIChaseState::Enter(AIContext& ctx) const
{
	if (ctx.intent)
		ctx.intent->ClearAll();
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
	ctx.intent->motion.hasLookTarget = true;
	ctx.intent->motion.lookTarget = ctx.blackboard->currentTarget;

	ctx.movementPolicy->BuildChaseIntent(ctx);
}