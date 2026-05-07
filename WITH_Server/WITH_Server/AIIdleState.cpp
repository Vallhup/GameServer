#include "pch.h"
#include "AIIdleState.h"

#include "IAIIdleActionPolicy.h"

void AIIdleState::Enter(AIContext& ctx) const
{
	if (ctx.intent)
		ctx.intent->ClearAll();
}

void AIIdleState::DecisionUpdate(AIContext& ctx, const double decisionDT) const
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

	ctx.idleActionPolicy->TryIssueIdleAction(ctx);
}

void AIIdleState::FrameUpdate(AIContext& ctx, const double dT) const
{
}
