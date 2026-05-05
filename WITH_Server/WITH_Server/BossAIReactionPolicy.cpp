#include "pch.h"
#include "BossAIReactionPolicy.h"

#include "IAIState.h"

ReactionDecision BossAIReactionPolicy::Evaluate(
	const AIReactionEvent& topEvent,
	const AIContext& ctx) const
{
	ReactionDecision decision{};

	switch (topEvent.type) {
	case AIReactionEventType::OnHitReceived:
		decision.outcome = ReactionTacticalOutcome::ForceRetarget;
		decision.retargetAttacker = true;
		break;

	case AIReactionEventType::OnParried:
	case AIReactionEventType::OnGuardBroken:
		decision.outcome = ReactionTacticalOutcome::EnterReact;
		decision.retargetAttacker = true;
		break;

	case AIReactionEventType::OnHpThreshold:
	default:
		decision.outcome = ReactionTacticalOutcome::Ignore;
		break;
	}

	return decision;
}
