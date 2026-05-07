#include "pch.h"
#include "AIReactState.h"

#include "AIBehaviorDef.h"
#include "IAIReactionPolicy.h"

void AIReactState::Enter(AIContext& ctx) const
{
	if (ctx.intent != nullptr)
		ctx.intent->ClearAll();
}

void AIReactState::Exit(AIContext& ctx) const
{
	if (ctx.decision != nullptr)
	{
		ctx.decision->reactDurationOverrideActive = false;
		ctx.decision->reactDurationOverrideSec = 0.0f;
	}
}

void AIReactState::DecisionUpdate(AIContext& ctx, const double decisionDT) const
{
	(void)decisionDT;

	const double reactDuration =
		ctx.decision != nullptr &&
		ctx.decision->reactDurationOverrideActive
		? static_cast<double>(ctx.decision->reactDurationOverrideSec)
		: ctx.decisionTuning->reactDuration;

	if (ctx.decision->stateTime < reactDuration)
		return;

	if (ctx.blackboard->returningHome)
	{
		ctx.decision->RequestTransition(AIStateType::ReturnHome);
		return;
	}

	if (ctx.perception->hasTarget)
	{
		ctx.decision->RequestTransition(AIStateType::Chase);
	}
	else
	{
		ctx.decision->RequestTransition(AIStateType::Idle);
	}
}

void AIReactState::FrameUpdate(AIContext& ctx, const double dT) const
{
	(void)ctx;
	(void)dT;
}
