#include "pch.h"
#include "AICombatActionSelector.h"

#include "AICombatActionConditionEvaluator.h"
#include "AICombatActionPolicyUtil.h"

AICombatActionChoice AICombatActionSelector::Select(
	const AICombatActionSelectionContext& context)
{
	const std::vector<AICombatActionCandidate> candidates =
		BuildPreferredCandidateSet(context);
	return PickWeightedCandidate(context, candidates);
}

bool AICombatActionSelector::IsEffectActionDue(
	const AICombatActionSelectionContext& context) noexcept
{
	if (context.basicActionCountSinceEffect == 0)
		return false;

	for (const AIActionDef& action : context.actions)
	{
		if (action.actionRole != AIActionRole::Effect ||
			action.requiresBasicActionCount == 0 ||
			context.basicActionCountSinceEffect <
				action.requiresBasicActionCount)
		{
			continue;
		}

		if (AICombatActionConditionEvaluator::MatchesPhase(
			context,
			action.condition))
		{
			return true;
		}
	}

	return false;
}

std::vector<AICombatActionCandidate>
AICombatActionSelector::CollectDueEffectCandidates(
	const AICombatActionSelectionContext& context,
	std::span<const AICombatActionCandidate> candidates)
{
	std::vector<AICombatActionCandidate> dueEffects;
	dueEffects.reserve(candidates.size());

	for (const AICombatActionCandidate& candidate : candidates)
	{
		if (candidate.actionIndex >= context.actions.size())
			continue;

		const AIActionDef& action =
			context.actions[candidate.actionIndex];
		if (action.actionRole == AIActionRole::Effect &&
			action.requiresBasicActionCount > 0 &&
			context.basicActionCountSinceEffect >=
				action.requiresBasicActionCount)
		{
			dueEffects.push_back(candidate);
		}
	}

	return dueEffects;
}

std::vector<AICombatActionCandidate>
AICombatActionSelector::BuildPreferredCandidateSet(
	const AICombatActionSelectionContext& context)
{
	const bool effectDue = IsEffectActionDue(context);

	std::vector<AICombatActionCandidate> candidates =
		AICombatActionConditionEvaluator::CollectEligibleCandidates(
			context,
			AIImmediateRepeatMode::ForbidConfiguredRepeats);
	if (effectDue)
	{
		std::vector<AICombatActionCandidate> dueEffects =
			CollectDueEffectCandidates(context, candidates);
		if (!dueEffects.empty())
			return dueEffects;
	}
	if (!candidates.empty())
		return candidates;

	candidates =
		AICombatActionConditionEvaluator::CollectEligibleCandidates(
			context,
			AIImmediateRepeatMode::AllowAsFallback);
	if (effectDue)
	{
		std::vector<AICombatActionCandidate> dueEffects =
			CollectDueEffectCandidates(context, candidates);
		if (!dueEffects.empty())
			return dueEffects;
	}

	return candidates;
}

AICombatActionChoice AICombatActionSelector::PickWeightedCandidate(
	const AICombatActionSelectionContext& context,
	std::span<const AICombatActionCandidate> candidates) noexcept
{
	uint32_t totalWeight{ 0 };
	for (const AICombatActionCandidate& candidate : candidates)
	{
		const uint32_t previousWeight = totalWeight;
		totalWeight += candidate.weight;
		if (totalWeight < previousWeight)
		{
			totalWeight = std::numeric_limits<uint32_t>::max();
			break;
		}
	}
	if (totalWeight == 0)
		return {};

	uint32_t cursor = static_cast<uint32_t>(std::max(
		0,
		AICombatActionPolicyUtil::PseudoRand(
			context.aiCtx.self.id,
			context.actionSequence,
			static_cast<int>(totalWeight))));

	for (const AICombatActionCandidate& candidate : candidates)
	{
		if (cursor < candidate.weight)
			return AICombatActionChoice{ candidate.actionIndex };

		cursor -= candidate.weight;
	}

	return {};
}
