#include "pch.h"
#include "DataDrivenAIReactionPolicy.h"

#include "ECS/Components/GameplayAIComponents.h"
#include "System.h"

ReactionDecision DataDrivenAIReactionPolicy::Evaluate(
	const AIReactionEvent& topEvent,
	const AIContext& ctx) const
{
	ReactionDecision decision{};

	if (ctx.behaviorProfile == nullptr)
		return decision;

	ReactionSelectionContext selection = BuildSelectionContext(topEvent, ctx);
	const AIReactionRuleDef* selectedRule = SelectRule(selection);

	if (selectedRule == nullptr)
		return decision;

	return BuildDecision(*selectedRule);
}

DataDrivenAIReactionPolicy::ReactionSelectionContext
DataDrivenAIReactionPolicy::BuildSelectionContext(
	const AIReactionEvent& topEvent,
	const AIContext& ctx) noexcept
{
	AIReactionRuleEvent	event{ AIReactionRuleEvent::OnHpThreshold };
	uint8_t				phase{ 1 };
	float				selfHpRatio{ 1.0f };

	// 1. event 변환
	{
		switch (topEvent.type) {
		case AIReactionEventType::OnHitReceived:
		{
			event = AIReactionRuleEvent::OnHitReceived;
			break;
		}
		case AIReactionEventType::OnParried:
		{
			event = AIReactionRuleEvent::OnParried;
			break;
		}
		case AIReactionEventType::OnGuardBroken:
		{
			event = AIReactionRuleEvent::OnGuardBroken;
			break;
		}
		case AIReactionEventType::OnHpThreshold:
		default:
		{
			event = AIReactionRuleEvent::OnHpThreshold;
			break;
		}
		}
	}

	// 2. phase 계산
	{
		if (ctx.sysCtx != nullptr)
		{
			if (const AIPhaseRuntimeComp* phaseComp =
				ctx.sysCtx->ecs.GetComponent<AIPhaseRuntimeComp>(ctx.self))
			{
				phase = std::max<uint8_t>(1, phaseComp->currentPhase);
			}
		}
	}

	// 3. self HP ratio 계산
	{
		if (ctx.stats != nullptr && ctx.stats->maxHp > 0)
		{
			const float rawHpRatio =
				static_cast<float>(ctx.stats->currentHp) /
				static_cast<float>(ctx.stats->maxHp);

			selfHpRatio = std::clamp(rawHpRatio, 0.0f, 1.0f);
		}
	}

	return ReactionSelectionContext
	{
		.aiCtx			= ctx,
		.rules			= ctx.behaviorProfile->reactionRules,
		.event			= event,
		.phase			= phase,
		.selfHpRatio	= selfHpRatio
	};
}

const AIReactionRuleDef* DataDrivenAIReactionPolicy::SelectRule(
	const ReactionSelectionContext& selection) noexcept
{
	const AIReactionRuleDef* selectedRule{ nullptr };

	for (const AIReactionRuleDef& rule : selection.rules)
	{
		// 1. event 일치 검사
		{
			if (rule.event != selection.event)
				continue;
		}

		// 2. phase 범위 검사
		{
			if (selection.phase < rule.phaseMin ||
				selection.phase > rule.phaseMax)
			{
				continue;
			}
		}

		// 3. self HP ratio 범위 검사
		{
			if (selection.selfHpRatio < rule.selfHpRatioMin ||
				selection.selfHpRatio > rule.selfHpRatioMax)
			{
				continue;
			}
		}

		// 4. priority 우선 선택
		{
			if (selectedRule == nullptr ||
				rule.priority > selectedRule->priority)
			{
				selectedRule = &rule;
			}
		}
	}

	return selectedRule;
}

ReactionDecision DataDrivenAIReactionPolicy::BuildDecision(
	const AIReactionRuleDef& rule) noexcept
{
	ReactionDecision decision{};

	// 1. outcome 변환
	{
		switch (rule.outcome) {
		case AIReactionRuleOutcome::ForceRetarget:
		{
			decision.outcome = ReactionTacticalOutcome::ForceRetarget;
			break;
		}
		case AIReactionRuleOutcome::EnterReact:
		{
			decision.outcome = ReactionTacticalOutcome::EnterReact;
			break;
		}
		case AIReactionRuleOutcome::IssueAbility:
		{
			decision.outcome = ReactionTacticalOutcome::IssueAbility;
			break;
		}
		case AIReactionRuleOutcome::EnterReactAndIssueAbility:
		{
			decision.outcome = ReactionTacticalOutcome::EnterReactAndIssueAbility;
			break;
		}
		case AIReactionRuleOutcome::Ignore:
		default:
		{
			decision.outcome = ReactionTacticalOutcome::Ignore;
			break;
		}
		}
	}

	// 2. retarget / abilityId 반영
	{
		decision.retargetAttacker	= rule.retargetAttacker;
		decision.reactAbilityId		= rule.reactAbilityId;
	}

	// 3. react duration override 반영
	{
		if (rule.reactDurationSec > 0.0f)
		{
			decision.hasReactDurationOverride	= true;
			decision.reactDurationSec			= rule.reactDurationSec;
		}
	}

	return decision;
}
