#include "pch.h"
#include "BossReactionPolicy.h"

#include "IAIState.h"

ReactionDecision BossReactionPolicy::Evaluate(
	const AIReactionEvent& topEvent,
	const AIContext& ctx) const
{
	(void)ctx;

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
