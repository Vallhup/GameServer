#include "pch.h"
#include "NormalAIIdleState.h"

void NormalAIIdleState::Enter(AIContext& ctx) const
{
	if (ctx.intent)
		ctx.intent->ClearAll();
}

void NormalAIIdleState::DecisionUpdate(AIContext& ctx, const double decisionDT) const
{
	if (ctx.perception->hasTarget)
	{
		ctx.decision->RequestTransition(AIStateType::Chase);
		return;
	}
}

void NormalAIIdleState::FrameUpdate(AIContext& ctx, const double dT) const
{
}