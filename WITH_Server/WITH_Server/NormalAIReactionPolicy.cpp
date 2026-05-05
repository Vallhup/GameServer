#include "pch.h"
#include "NormalAIReactionPolicy.h"

#include "IAIState.h"

ReactionDecision NormalAIReactionPolicy::Evaluate(
	const AIReactionEvent& topEvent,
	const AIContext&       ctx) const
{
	ReactionDecision decision{};

	switch (topEvent.type) {
	case AIReactionEventType::OnHitReceived:
	{
		decision.outcome = ReactionTacticalOutcome::EnterReact;
		decision.retargetAttacker = true;
		break;
	}
	case AIReactionEventType::OnParried:
	{
		// 패리당한 경우: 반응 진입 + 패리한 적을 즉시 타겟으로 전환
		decision.outcome = ReactionTacticalOutcome::EnterReact;
		decision.retargetAttacker = true;
		break;
	}
	case AIReactionEventType::OnGuardBroken:
	{
		decision.outcome = ReactionTacticalOutcome::EnterReact;
		decision.retargetAttacker = false;
		break;
	}
	case AIReactionEventType::OnHpThreshold:
	{
		// 일반 잡몹은 HP 임계값 이벤트를 무시한다 (보스 전용)
		decision.outcome = ReactionTacticalOutcome::Ignore;
		break;
	}
	default:
	{
		decision.outcome = ReactionTacticalOutcome::Ignore;
		break;
	}
	}

	return decision;
}
