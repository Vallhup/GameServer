#include "pch.h"
#include "DataDrivenAIReactionPolicy.h"

#include "AIBehaviorDef.h"
#include "IAIState.h"
#include "System.h"

#include <algorithm>

namespace
{
	AIReactionRuleEvent ToRuleEvent(AIReactionEventType type) noexcept
	{
		switch (type) {
		case AIReactionEventType::OnHitReceived:
			return AIReactionRuleEvent::OnHitReceived;
		case AIReactionEventType::OnParried:
			return AIReactionRuleEvent::OnParried;
		case AIReactionEventType::OnGuardBroken:
			return AIReactionRuleEvent::OnGuardBroken;
		case AIReactionEventType::OnHpThreshold:
		default:
			return AIReactionRuleEvent::OnHpThreshold;
		}
	}

	ReactionTacticalOutcome ToTacticalOutcome(
		AIReactionRuleOutcome outcome) noexcept
	{
		switch (outcome) {
		case AIReactionRuleOutcome::ForceRetarget:
			return ReactionTacticalOutcome::ForceRetarget;
		case AIReactionRuleOutcome::EnterReact:
			return ReactionTacticalOutcome::EnterReact;
		case AIReactionRuleOutcome::IssueAbility:
			return ReactionTacticalOutcome::IssueAbility;
		case AIReactionRuleOutcome::EnterReactAndIssueAbility:
			return ReactionTacticalOutcome::EnterReactAndIssueAbility;
		case AIReactionRuleOutcome::Ignore:
		default:
			return ReactionTacticalOutcome::Ignore;
		}
	}

	uint8_t ResolvePhase(const AIContext& ctx) noexcept
	{
		if (ctx.sysCtx == nullptr)
			return 1;

		const AIPhaseRuntimeComp* phase =
			ctx.sysCtx->ecs.GetComponent<AIPhaseRuntimeComp>(ctx.self);
		return phase ? std::max<uint8_t>(1, phase->currentPhase) : 1;
	}

	float ResolveHpRatio(const CombatStatStateComp* stats) noexcept
	{
		if (stats == nullptr || stats->maxHp <= 0)
			return 1.0f;

		return std::clamp(
			static_cast<float>(stats->currentHp) /
				static_cast<float>(stats->maxHp),
			0.0f,
			1.0f);
	}
}

ReactionDecision DataDrivenAIReactionPolicy::Evaluate(
	const AIReactionEvent& topEvent,
	const AIContext& ctx) const
{
	ReactionDecision decision{};
	if (ctx.behaviorProfile == nullptr)
		return decision;

	const AIReactionRuleEvent event = ToRuleEvent(topEvent.type);
	const uint8_t phase = ResolvePhase(ctx);
	const float hpRatio = ResolveHpRatio(ctx.stats);

	const AIReactionRuleDef* selectedRule = nullptr;
	for (const AIReactionRuleDef& rule : ctx.behaviorProfile->reactionRules)
	{
		if (rule.event != event)
			continue;
		if (phase < rule.phaseMin || phase > rule.phaseMax)
			continue;
		if (hpRatio < rule.selfHpRatioMin ||
			hpRatio > rule.selfHpRatioMax)
		{
			continue;
		}

		if (selectedRule == nullptr ||
			rule.priority > selectedRule->priority)
		{
			selectedRule = &rule;
		}
	}

	if (selectedRule == nullptr)
		return decision;

	decision.outcome = ToTacticalOutcome(selectedRule->outcome);
	decision.retargetAttacker = selectedRule->retargetAttacker;
	decision.reactAbilityId = selectedRule->reactAbilityId;
	if (selectedRule->reactDurationSec > 0.0f)
	{
		decision.hasReactDurationOverride = true;
		decision.reactDurationSec = selectedRule->reactDurationSec;
	}
	return decision;
}
