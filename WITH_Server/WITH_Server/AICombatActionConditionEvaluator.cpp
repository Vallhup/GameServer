#include "pch.h"
#include "AICombatActionConditionEvaluator.h"

#include "ECS/System/AbilityProfileService.h"
#include "System.h"

std::vector<AICombatActionCandidate>
AICombatActionConditionEvaluator::CollectEligibleCandidates(
	const AICombatActionSelectionContext& context,
	AIImmediateRepeatMode repeatMode)
{
	std::vector<AICombatActionCandidate> candidates;
	candidates.reserve(context.actions.size());

	for (size_t actionIndex = 0;
		actionIndex < context.actions.size();
		++actionIndex)
	{
		const AIActionDef& action = context.actions[actionIndex];
		const AIActionConditionDef& condition = action.condition;

		const bool matchesDefinitionConditions =
			MatchesPhase(context, condition) &&
			MatchesSpatialConditions(context, condition) &&
			MatchesHealthConditions(context, condition) &&
			MatchesPerceptionConditions(context, condition);
		if (!matchesDefinitionConditions)
			continue;

		const bool matchesRuntimeConditions =
			MatchesAbilityConditions(context, action) &&
			MatchesCooldownConditions(context, action, actionIndex) &&
			MatchesActionSequenceConditions(context, action);
		if (!matchesRuntimeConditions)
			continue;

		const uint32_t weight =
			ResolveWeight(context, action, repeatMode);
		if (weight == 0)
			continue;

		candidates.push_back(AICombatActionCandidate{
			.actionIndex = actionIndex,
			.weight = weight
		});
	}

	return candidates;
}

bool AICombatActionConditionEvaluator::MatchesPhase(
	const AICombatActionSelectionContext& context,
	const AIActionConditionDef& condition) noexcept
{
	return
		context.phase >= condition.phaseMin &&
		context.phase <= condition.phaseMax;
}

bool AICombatActionConditionEvaluator::MatchesSpatialConditions(
	const AICombatActionSelectionContext& context,
	const AIActionConditionDef& condition) noexcept
{
	if (condition.distanceBucket != AIActionDistanceBucket::Any &&
		condition.distanceBucket != context.currentBucket)
	{
		return false;
	}

	if (condition.minDistance.has_value() &&
		context.distanceToTarget <
			static_cast<double>(*condition.minDistance))
	{
		return false;
	}

	return
		!condition.maxDistance.has_value() ||
		context.distanceToTarget <=
			static_cast<double>(*condition.maxDistance);
}

bool AICombatActionConditionEvaluator::MatchesHealthConditions(
	const AICombatActionSelectionContext& context,
	const AIActionConditionDef& condition) noexcept
{
	return
		context.selfHpRatio >= condition.selfHpRatioMin &&
		context.selfHpRatio <= condition.selfHpRatioMax &&
		context.targetHpRatio >= condition.targetHpRatioMin &&
		context.targetHpRatio <= condition.targetHpRatioMax;
}

bool AICombatActionConditionEvaluator::MatchesPerceptionConditions(
	const AICombatActionSelectionContext& context,
	const AIActionConditionDef& condition) noexcept
{
	const AIPerceptionComp* perception = context.aiCtx.perception;
	if (condition.requiresTargetVisible &&
		(perception == nullptr || !perception->targetVisible))
	{
		return false;
	}

	return
		!condition.requiresTargetInFront ||
		(perception != nullptr && perception->targetInFront);
}

bool AICombatActionConditionEvaluator::MatchesAbilityConditions(
	const AICombatActionSelectionContext& context,
	const AIActionDef& action) noexcept
{
	if (!action.condition.requiresAbilityAvailable)
		return true;

	if (context.aiCtx.abilityState == nullptr ||
		!context.aiCtx.abilityState->CanIssueAbility() ||
		action.abilityId == InvalidAbilityId ||
		context.aiCtx.sysCtx == nullptr)
	{
		return false;
	}

	const SpawnTypeComp* spawnType =
		context.aiCtx.sysCtx->ecs.GetComponent<SpawnTypeComp>(
			context.aiCtx.self);
	return
		spawnType != nullptr &&
		AbilityProfileService::IsAbilityAvailable(
			spawnType->characterId,
			action.abilityId);
}

bool AICombatActionConditionEvaluator::MatchesCooldownConditions(
	const AICombatActionSelectionContext& context,
	const AIActionDef& action,
	size_t actionIndex) noexcept
{
	if (context.runtime == nullptr)
		return true;

	if (actionIndex < context.runtime->actionCooldownSec.size() &&
		context.runtime->actionCooldownSec[actionIndex] > 0.0f)
	{
		return false;
	}

	if (action.groupId == InvalidAIActionGroupId)
		return true;

	const size_t groupIndex =
		static_cast<size_t>(action.groupId - 1u);
	return
		groupIndex >= context.runtime->groupCooldownSec.size() ||
		context.runtime->groupCooldownSec[groupIndex] <= 0.0f;
}

bool AICombatActionConditionEvaluator::MatchesActionSequenceConditions(
	const AICombatActionSelectionContext& context,
	const AIActionDef& action) noexcept
{
	return
		action.actionRole != AIActionRole::Effect ||
		action.requiresBasicActionCount == 0 ||
		context.basicActionCountSinceEffect >=
			action.requiresBasicActionCount;
}

uint32_t AICombatActionConditionEvaluator::ResolveWeight(
	const AICombatActionSelectionContext& context,
	const AIActionDef& action,
	AIImmediateRepeatMode repeatMode) noexcept
{
	if (action.weight == 0)
		return 0;

	if (action.abilityId != context.lastUsedAbilityId)
		return action.weight;

	if (action.forbidImmediateRepeat)
	{
		return repeatMode == AIImmediateRepeatMode::AllowAsFallback
			? action.weight
			: 0u;
	}

	const float adjustedWeight =
		static_cast<float>(action.weight) *
		action.repeatWeightMultiplier;
	if (adjustedWeight <= 0.0f)
		return 0;

	return static_cast<uint32_t>(std::floor(adjustedWeight));
}
