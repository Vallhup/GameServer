#include "pch.h"
#include "DataDrivenAICombatActionPolicy.h"

#include "AIBehaviorDef.h"
#include "AICombatActionPolicyUtil.h"
#include "ECS/System/AbilityProfileService.h"
#include "IAIState.h"
#include "System.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
	struct ActionCandidate
	{
		size_t actionIndex{ 0 };
		uint32_t weight{ 0 };
	};

	uint8_t ResolvePhase(const AIContext& ctx) noexcept
	{
		if (ctx.sysCtx == nullptr)
			return 1;

		const AIPhaseRuntimeComp* phase =
			ctx.sysCtx->ecs.GetComponent<AIPhaseRuntimeComp>(ctx.self);
		return phase ? std::max<uint8_t>(1, phase->currentPhase) : 1;
	}

	const AIMovementProfileDef* SelectMovementProfile(
		const AIBehaviorProfileDef& profile,
		uint8_t phase) noexcept
	{
		for (const AIMovementProfileDef& movement : profile.movementProfiles)
		{
			if (phase >= movement.phaseMin && phase <= movement.phaseMax)
				return &movement;
		}

		return profile.movementProfiles.empty()
			? nullptr
			: &profile.movementProfiles.front();
	}

	AIActionDistanceBucket ResolveDistanceBucket(
		double distance,
		const AIMovementProfileDef* movement) noexcept
	{
		if (movement == nullptr)
			return AIActionDistanceBucket::Any;

		if (distance <= movement->veryCloseDistance)
			return AIActionDistanceBucket::VeryClose;
		if (distance <= movement->closeDistance)
			return AIActionDistanceBucket::Close;
		if (distance <= movement->midDistance)
			return AIActionDistanceBucket::Mid;
		return AIActionDistanceBucket::Far;
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

	const CombatStatStateComp* ResolveTargetStats(const AIContext& ctx) noexcept
	{
		if (ctx.sysCtx == nullptr || ctx.blackboard == nullptr)
			return nullptr;

		Entity target = Entity::Null();
		if (ctx.perception != nullptr && !ctx.perception->selectedTarget.IsNull())
			target = ctx.perception->selectedTarget;
		else if (!ctx.blackboard->currentTarget.IsNull())
			target = ctx.blackboard->currentTarget;

		return target.IsNull()
			? nullptr
			: ctx.sysCtx->ecs.GetComponent<CombatStatStateComp>(target);
	}

	bool IsAbilityAvailableForSelf(
		const AIContext& ctx,
		AbilityId abilityId) noexcept
	{
		if (abilityId == InvalidAbilityId || ctx.sysCtx == nullptr)
			return false;

		const SpawnTypeComp* spawnType =
			ctx.sysCtx->ecs.GetComponent<SpawnTypeComp>(ctx.self);
		if (spawnType == nullptr)
			return false;

		return AbilityProfileService::IsAbilityAvailable(
			spawnType->characterId,
			abilityId);
	}

	bool IsActionCooldownReady(
		const AIActionRuntimeComp* runtime,
		size_t actionIndex) noexcept
	{
		return
			runtime == nullptr ||
			actionIndex >= runtime->actionCooldownSec.size() ||
			runtime->actionCooldownSec[actionIndex] <= 0.0f;
	}

	bool IsGroupCooldownReady(
		const AIActionRuntimeComp* runtime,
		const AIActionDef& action) noexcept
	{
		if (runtime == nullptr ||
			action.groupId == InvalidAIActionGroupId)
		{
			return true;
		}

		const size_t index = static_cast<size_t>(action.groupId - 1u);
		return
			index >= runtime->groupCooldownSec.size() ||
			runtime->groupCooldownSec[index] <= 0.0f;
	}

	bool MatchesDistance(
		const AIActionDef& action,
		double distance,
		AIActionDistanceBucket currentBucket) noexcept
	{
		const AIActionConditionDef& condition = action.condition;
		if (condition.distanceBucket != AIActionDistanceBucket::Any &&
			condition.distanceBucket != currentBucket)
		{
			return false;
		}

		if (condition.minDistance.has_value() &&
			distance < static_cast<double>(*condition.minDistance))
		{
			return false;
		}

		if (condition.maxDistance.has_value() &&
			distance > static_cast<double>(*condition.maxDistance))
		{
			return false;
		}

		return true;
	}

	bool MatchesCondition(
		const AIContext& ctx,
		const AIActionDef& action,
		uint8_t phase,
		AIActionDistanceBucket currentBucket,
		float selfHpRatio,
		float targetHpRatio) noexcept
	{
		const AIActionConditionDef& condition = action.condition;
		if (phase < condition.phaseMin || phase > condition.phaseMax)
			return false;

		const double distance = ctx.perception != nullptr
			? ctx.perception->distanceToTarget
			: std::numeric_limits<double>::max();
		if (!MatchesDistance(action, distance, currentBucket))
			return false;

		if (selfHpRatio < condition.selfHpRatioMin ||
			selfHpRatio > condition.selfHpRatioMax ||
			targetHpRatio < condition.targetHpRatioMin ||
			targetHpRatio > condition.targetHpRatioMax)
		{
			return false;
		}

		if (condition.requiresTargetVisible &&
			(ctx.perception == nullptr || !ctx.perception->targetVisible))
		{
			return false;
		}

		if (condition.requiresTargetInFront &&
			(ctx.perception == nullptr || !ctx.perception->targetInFront))
		{
			return false;
		}

		if (condition.requiresAbilityAvailable)
		{
			if (ctx.abilityState == nullptr ||
				!ctx.abilityState->CanIssueAbility() ||
				!IsAbilityAvailableForSelf(ctx, action.abilityId))
			{
				return false;
			}
		}

		return true;
	}

	AbilityId ResolveLastUsedAbility(const AIContext& ctx) noexcept
	{
		if (ctx.actionRuntime != nullptr)
			return ctx.actionRuntime->lastUsedAbilityId;

		return ctx.actionRuntime != nullptr
			? ctx.actionRuntime->lastUsedAbilityId
			: InvalidAbilityId;
	}

	uint32_t ResolveActionSequence(const AIContext& ctx) noexcept
	{
		return ctx.actionRuntime != nullptr
			? ctx.actionRuntime->actionSequence
			: 0u;
	}

	uint32_t ResolveAdjustedWeight(
		const AIActionDef& action,
		AbilityId lastUsed) noexcept
	{
		if (action.weight == 0)
			return 0;

		if (action.abilityId != lastUsed)
			return action.weight;

		if (action.forbidImmediateRepeat)
			return 0;

		const float adjusted =
			static_cast<float>(action.weight) * action.repeatWeightMultiplier;
		return adjusted <= 0.0f
			? 0u
			: static_cast<uint32_t>(std::floor(adjusted));
	}

	const AIActionDef* PickAction(
		const AIContext& ctx,
		std::span<const AIActionDef> actions,
		const std::vector<ActionCandidate>& candidates,
		size_t& outActionIndex) noexcept
	{
		uint32_t totalWeight{ 0 };
		for (const ActionCandidate& candidate : candidates)
		{
			totalWeight += candidate.weight;
		}

		if (totalWeight == 0)
			return nullptr;

		const int roll = AICombatActionPolicyUtil::PseudoRand(
			ctx.self.id,
			ResolveActionSequence(ctx),
			static_cast<int>(totalWeight));
		uint32_t cursor = static_cast<uint32_t>(std::max(0, roll));

		for (const ActionCandidate& candidate : candidates)
		{
			if (cursor < candidate.weight)
			{
				outActionIndex = candidate.actionIndex;
				return &actions[candidate.actionIndex];
			}

			cursor -= candidate.weight;
		}

		return nullptr;
	}

	void EnsureRuntimeSlots(
		AIActionRuntimeComp& runtime,
		size_t actionIndex,
		AIActionGroupId groupId)
	{
		if (actionIndex >= runtime.actionCooldownSec.size())
		{
			runtime.actionCooldownSec.resize(actionIndex + 1u, 0.0f);
		}

		if (groupId != InvalidAIActionGroupId)
		{
			const size_t groupIndex = static_cast<size_t>(groupId - 1u);
			if (groupIndex >= runtime.groupCooldownSec.size())
			{
				runtime.groupCooldownSec.resize(groupIndex + 1u, 0.0f);
			}
		}
	}

	void ApplyRuntimeSelection(
		AIContext const& ctx,
		const AIActionDef& action,
		size_t actionIndex)
	{
		AIActionRuntimeComp* runtime = ctx.actionRuntime;
		if (runtime == nullptr && ctx.sysCtx != nullptr)
		{
			runtime =
				ctx.sysCtx->ecs.GetMutableComponent<AIActionRuntimeComp>(ctx.self);
		}
		if (runtime == nullptr)
			return;

		EnsureRuntimeSlots(*runtime, actionIndex, action.groupId);
		runtime->actionCooldownSec[actionIndex] = action.aiCooldownSec;
		if (action.groupId != InvalidAIActionGroupId)
		{
			runtime->groupCooldownSec[
				static_cast<size_t>(action.groupId - 1u)] =
				action.groupCooldownSec;
		}
		runtime->globalActionCooldownSec = action.globalCooldownSec;
		runtime->movementLockSec = action.lockMovementSec;
		runtime->lastUsedAbilityId = action.abilityId;
		++runtime->actionSequence;
	}
}

CombatActionSelection DataDrivenAICombatActionPolicy::SelectAction(
	const AIContext& ctx) const
{
	CombatActionSelection result{};
	if (ctx.behaviorProfile == nullptr ||
		ctx.behaviorProfile->combatActionDefs.empty() ||
		ctx.perception == nullptr ||
		!ctx.perception->hasTarget)
	{
		return result;
	}

	const AIActionRuntimeComp* runtime = ctx.actionRuntime;
	if (runtime != nullptr && runtime->globalActionCooldownSec > 0.0f)
	{
		return result;
	}

	const uint8_t phase = ResolvePhase(ctx);
	const AIMovementProfileDef* movement =
		SelectMovementProfile(*ctx.behaviorProfile, phase);
	const AIActionDistanceBucket currentBucket =
		ResolveDistanceBucket(ctx.perception->distanceToTarget, movement);
	const float selfHpRatio = ResolveHpRatio(ctx.stats);
	const float targetHpRatio = ResolveHpRatio(ResolveTargetStats(ctx));
	const AbilityId lastUsed = ResolveLastUsedAbility(ctx);

	std::vector<ActionCandidate> candidates;
	candidates.reserve(ctx.behaviorProfile->combatActionDefs.size());

	for (size_t i = 0; i < ctx.behaviorProfile->combatActionDefs.size(); ++i)
	{
		const AIActionDef& action = ctx.behaviorProfile->combatActionDefs[i];
		if (!MatchesCondition(
				ctx,
				action,
				phase,
				currentBucket,
				selfHpRatio,
				targetHpRatio))
		{
			continue;
		}

		if (!IsActionCooldownReady(runtime, i) ||
			!IsGroupCooldownReady(runtime, action))
		{
			continue;
		}

		const uint32_t weight = ResolveAdjustedWeight(action, lastUsed);
		if (weight == 0)
			continue;

		candidates.push_back(ActionCandidate{
			.actionIndex = i,
			.weight = weight
		});
	}

	size_t selectedIndex{ 0 };
	const AIActionDef* selected = PickAction(
		ctx,
		ctx.behaviorProfile->combatActionDefs,
		candidates,
		selectedIndex);
	if (selected == nullptr)
		return result;

	result.shouldAttack = true;
	result.selectedAbilityId = selected->abilityId;
	result.target = ctx.perception->selectedTarget;
	AICombatActionPolicyUtil::FillAttackDirectionTowardTarget(ctx, result);
	ApplyRuntimeSelection(ctx, *selected, selectedIndex);
	return result;
}
