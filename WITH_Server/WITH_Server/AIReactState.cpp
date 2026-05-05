#include "pch.h"
#include "AIReactState.h"

#include "AIBehaviorDef.h"
#include "IAIReactionPolicy.h"

// AIDecisionSystem::RunFSM 에서 이미 policy 판정 후 Enter 가 호출된다.
// Enter 에서는 blackboard 를 업데이트하고 command 를 초기화하는 것으로 충분하다.

void AIReactState::Enter(AIContext& ctx) const
{
	if (ctx.command)
		ctx.command->ClearAll();
}

void AIReactState::DecisionUpdate(AIContext& ctx, const double decisionDT) const
{
	// reactDuration 이 지난 후 다음 상태로 전환
	const double reactDuration = ctx.decisionTuning->reactDuration;

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
	// React 상태에서는 움직임 없음 — 피격 애니메이션만 재생
}
