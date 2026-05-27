#include "pch.h"
#include "DataDrivenAICombatActionPolicy.h"

#include "AICombatActionPolicyUtil.h"
#include "ECS/Components/GameplayAIComponents.h"
#include "ECS/System/AbilityProfileService.h"
#include "System.h"

CombatActionSelection DataDrivenAICombatActionPolicy::SelectAction(
	const AIContext& ctx) const
{
	CombatActionSelection result{};

	if (!CanSelectCombatAction(ctx))
	{
		return result;
	}

	CombatActionSelectionContext selection = BuildSelectionContext(ctx);
	std::vector<CombatActionCandidate> candidates = CollectCandidates(selection);
	if (IsEffectActionDue(selection))
	{
		KeepDueEffectCandidatesOnly(selection, candidates);
	}

	size_t selectedActionIndex{ 0 };
	const AIActionDef* selectedAction =
		PickCombatAction(selection, candidates, selectedActionIndex);

	if (selectedAction == nullptr)
		return result;

	// 1. 선택 결과 채우기
	{
		result.shouldAttack			= true;
		result.selectedAbilityId	= selectedAction->abilityId;
		result.target				= ctx.perception->selectedTarget;

		AICombatActionPolicyUtil::FillAttackDirectionTowardTarget(ctx, result);
	}

	// 2. runtime 상태 반영
	{
		ApplyRuntimeSelection(selection, *selectedAction, selectedActionIndex);
	}

	return result;
}

bool DataDrivenAICombatActionPolicy::CanSelectCombatAction(
	const AIContext& ctx) noexcept
{
	if (ctx.behaviorProfile == nullptr ||
		ctx.behaviorProfile->combatActionDefs.empty() ||
		ctx.perception == nullptr ||
		!ctx.perception->hasTarget)
	{
		return false;
	}

	if (ctx.actionRuntime != nullptr &&
		ctx.actionRuntime->globalActionCooldownSec > 0.0f)
	{
		return false;
	}

	return true;
}

DataDrivenAICombatActionPolicy::CombatActionSelectionContext
DataDrivenAICombatActionPolicy::BuildSelectionContext(
	const AIContext& ctx) noexcept
{
	uint8_t						phase{ 1 };
	const AIMovementProfileDef*	movement{ nullptr };
	AIActionDistanceBucket		currentBucket{ AIActionDistanceBucket::Any };
	float						targetHpRatio{ 1.0f };

	const double distanceToTarget = ctx.perception != nullptr
		? ctx.perception->distanceToTarget
		: std::numeric_limits<double>::max();

	// 1. phase 계산
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

	// 2. movement profile 선택 (phase 기준)
	{
		for (const AIMovementProfileDef& profile :
			ctx.behaviorProfile->movementProfiles)
		{
			if (phase >= profile.phaseMin && phase <= profile.phaseMax)
			{
				movement = &profile;
				break;
			}
		}

		if (movement == nullptr &&
			!ctx.behaviorProfile->movementProfiles.empty())
		{
			movement = &ctx.behaviorProfile->movementProfiles.front();
		}
	}

	// 3. distance bucket 계산
	{
		if (movement != nullptr)
		{
			if (distanceToTarget <= movement->veryCloseDistance)
				currentBucket = AIActionDistanceBucket::VeryClose;
			else if (distanceToTarget <= movement->closeDistance)
				currentBucket = AIActionDistanceBucket::Close;
			else if (distanceToTarget <= movement->midDistance)
				currentBucket = AIActionDistanceBucket::Mid;
			else
				currentBucket = AIActionDistanceBucket::Far;
		}
	}

	// 4. target HP ratio 계산
	{
		Entity target = Entity::Null();

		if (ctx.perception != nullptr &&
			!ctx.perception->selectedTarget.IsNull())
		{
			target = ctx.perception->selectedTarget;
		}
		else if (ctx.blackboard != nullptr &&
			!ctx.blackboard->currentTarget.IsNull())
		{
			target = ctx.blackboard->currentTarget;
		}

		if (!target.IsNull() && ctx.sysCtx != nullptr)
		{
			targetHpRatio = ResolveHpRatio(
				ctx.sysCtx->ecs.GetComponent<CombatStatStateComp>(target));
		}
	}

	return CombatActionSelectionContext
	{
		.aiCtx				= ctx,
		.runtime			= ctx.actionRuntime,
		.actions			= ctx.behaviorProfile->combatActionDefs,
		.movement			= movement,
		.currentBucket		= currentBucket,
		.distanceToTarget	= distanceToTarget,
		.phase				= phase,
		.selfHpRatio		= ResolveHpRatio(ctx.stats),
		.targetHpRatio		= targetHpRatio,
		.lastUsedAbilityId	= ctx.actionRuntime != nullptr
			? ctx.actionRuntime->lastUsedAbilityId
			: InvalidAbilityId,
		.actionSequence		= ctx.actionRuntime != nullptr
			? ctx.actionRuntime->actionSequence
			: 0u,
		.basicActionCountSinceEffect = ctx.actionRuntime != nullptr
			? ctx.actionRuntime->basicActionCountSinceEffect
			: 0u
	};
}

float DataDrivenAICombatActionPolicy::ResolveHpRatio(
	const CombatStatStateComp* stats) noexcept
{
	if (stats == nullptr || stats->maxHp <= 0)
		return 1.0f;

	const float rawHpRatio =
		static_cast<float>(stats->currentHp) /
		static_cast<float>(stats->maxHp);

	return std::clamp(rawHpRatio, 0.0f, 1.0f);
}

std::vector<DataDrivenAICombatActionPolicy::CombatActionCandidate>
DataDrivenAICombatActionPolicy::CollectCandidates(
	const CombatActionSelectionContext& selection)
{
	std::vector<CombatActionCandidate> candidates;
	candidates.reserve(selection.actions.size());

	for (size_t i = 0; i < selection.actions.size(); ++i)
	{
		const AIActionDef& action = selection.actions[i];
		const AIActionConditionDef& condition = action.condition;

		// 1. phase 조건 검사
		{
			if (selection.phase < condition.phaseMin ||
				selection.phase > condition.phaseMax)
			{
				continue;
			}
		}

		// 2. distance bucket / min/max distance 조건 검사
		{
			if (condition.distanceBucket != AIActionDistanceBucket::Any &&
				condition.distanceBucket != selection.currentBucket)
			{
				continue;
			}

			if (condition.minDistance.has_value() &&
				selection.distanceToTarget <
				static_cast<double>(*condition.minDistance))
			{
				continue;
			}

			if (condition.maxDistance.has_value() &&
				selection.distanceToTarget >
				static_cast<double>(*condition.maxDistance))
			{
				continue;
			}
		}

		// 3. self / target HP ratio 조건 검사
		{
			if (selection.selfHpRatio < condition.selfHpRatioMin ||
				selection.selfHpRatio > condition.selfHpRatioMax ||
				selection.targetHpRatio < condition.targetHpRatioMin ||
				selection.targetHpRatio > condition.targetHpRatioMax)
			{
				continue;
			}
		}

		// 4. target perception 조건 검사
		{
			if (condition.requiresTargetVisible)
			{
				if (selection.aiCtx.perception == nullptr ||
					!selection.aiCtx.perception->targetVisible)
				{
					continue;
				}
			}

			if (condition.requiresTargetInFront)
			{
				if (selection.aiCtx.perception == nullptr ||
					!selection.aiCtx.perception->targetInFront)
				{
					continue;
				}
			}
		}

		// 5. ability 사용 가능 조건 검사
		{
			if (condition.requiresAbilityAvailable)
			{
				if (selection.aiCtx.abilityState == nullptr ||
					!selection.aiCtx.abilityState->CanIssueAbility())
				{
					continue;
				}

				if (action.abilityId == InvalidAbilityId ||
					selection.aiCtx.sysCtx == nullptr)
				{
					continue;
				}

				const SpawnTypeComp* spawnType =
					selection.aiCtx.sysCtx->ecs.GetComponent<SpawnTypeComp>(
						selection.aiCtx.self);

				if (spawnType == nullptr)
					continue;

				if (!AbilityProfileService::IsAbilityAvailable(
					spawnType->characterId,
					action.abilityId))
				{
					continue;
				}
			}
		}

		// 6. action cooldown 검사
		{
			if (selection.runtime != nullptr &&
				i < selection.runtime->actionCooldownSec.size() &&
				selection.runtime->actionCooldownSec[i] > 0.0f)
			{
				continue;
			}
		}

		// 7. group cooldown 검사
		{
			if (selection.runtime != nullptr &&
				action.groupId != InvalidAIActionGroupId)
			{
				const size_t groupIndex =
					static_cast<size_t>(action.groupId - 1u);

				if (groupIndex <
					selection.runtime->groupCooldownSec.size() &&
					selection.runtime->groupCooldownSec[groupIndex] > 0.0f)
				{
					continue;
				}
			}
		}

		// 8. weight 계산
		uint32_t weight = action.weight;
		{
			if (weight == 0)
				continue;

			if (action.abilityId == selection.lastUsedAbilityId)
			{
				if (action.forbidImmediateRepeat)
					continue;

				const float adjustedWeight =
					static_cast<float>(weight) *
					action.repeatWeightMultiplier;

				if (adjustedWeight <= 0.0f)
					continue;

				weight =
					static_cast<uint32_t>(std::floor(adjustedWeight));

				if (weight == 0)
					continue;
			}
		}

		candidates.push_back(CombatActionCandidate
			{
				.actionIndex	= i,
				.weight			= weight
			});
	}

	return candidates;
}

bool DataDrivenAICombatActionPolicy::IsEffectActionDue(
	const CombatActionSelectionContext& selection) noexcept
{
	if (selection.basicActionCountSinceEffect == 0)
		return false;

	for (const AIActionDef& action : selection.actions)
	{
		if (action.actionRole != AIActionRole::Effect ||
			action.requiresBasicActionCount == 0 ||
			selection.basicActionCountSinceEffect <
				action.requiresBasicActionCount)
		{
			continue;
		}

		const AIActionConditionDef& condition = action.condition;
		if (selection.phase >= condition.phaseMin &&
			selection.phase <= condition.phaseMax)
		{
			return true;
		}
	}

	return false;
}

void DataDrivenAICombatActionPolicy::KeepDueEffectCandidatesOnly(
	const CombatActionSelectionContext& selection,
	std::vector<CombatActionCandidate>& candidates)
{
	std::erase_if(
		candidates,
		[&selection](const CombatActionCandidate& candidate)
		{
			if (candidate.actionIndex >= selection.actions.size())
				return true;

			const AIActionDef& action =
				selection.actions[candidate.actionIndex];
			return action.actionRole != AIActionRole::Effect ||
				action.requiresBasicActionCount == 0 ||
				selection.basicActionCountSinceEffect <
					action.requiresBasicActionCount;
		});
}

const AIActionDef* DataDrivenAICombatActionPolicy::PickCombatAction(
	const CombatActionSelectionContext& selection,
	std::span<const CombatActionCandidate> candidates,
	size_t& outActionIndex) noexcept
{
	outActionIndex = 0;

	if (candidates.empty())
		return nullptr;

	uint32_t totalWeight{ 0 };

	// 1. total weight 계산
	{
		for (const CombatActionCandidate& candidate : candidates)
		{
			if (candidate.weight == 0)
				continue;

			const uint32_t prevWeight = totalWeight;
			totalWeight += candidate.weight;

			if (totalWeight < prevWeight)
			{
				totalWeight = std::numeric_limits<uint32_t>::max();
				break;
			}
		}

		if (totalWeight == 0)
			return nullptr;
	}

	// 2. weighted cursor 계산
	uint32_t cursor{ 0 };
	{
		const int roll =
			AICombatActionPolicyUtil::PseudoRand(
				selection.aiCtx.self.id,
				selection.actionSequence,
				static_cast<int>(totalWeight));

		cursor = static_cast<uint32_t>(std::max(0, roll));
	}

	// 3. cursor 위치에 해당하는 candidate 선택
	{
		for (const CombatActionCandidate& candidate : candidates)
		{
			if (cursor < candidate.weight)
			{
				outActionIndex =
					static_cast<size_t>(candidate.actionIndex);

				if (outActionIndex >= selection.actions.size())
					return nullptr;

				return &selection.actions[outActionIndex];
			}

			cursor -= candidate.weight;
		}
	}

	return nullptr;
}

void DataDrivenAICombatActionPolicy::ApplyRuntimeSelection(
	const CombatActionSelectionContext& selection,
	const AIActionDef& action,
	size_t actionIndex)
{
	AIActionRuntimeComp* runtime = selection.runtime;

	// 1. runtime 컴포넌트 확보
	{
		if (runtime == nullptr && selection.aiCtx.sysCtx != nullptr)
		{
			runtime =
				selection.aiCtx.sysCtx->ecs.GetMutableComponent<AIActionRuntimeComp>(
					selection.aiCtx.self);
		}

		if (runtime == nullptr)
			return;
	}

	// 2. action cooldown slot 보장 및 반영
	{
		if (actionIndex >= runtime->actionCooldownSec.size())
		{
			runtime->actionCooldownSec.resize(
				actionIndex + 1u,
				0.0f);
		}

		runtime->actionCooldownSec[actionIndex] = action.aiCooldownSec;
	}

	// 3. group cooldown slot 보장 및 반영
	{
		if (action.groupId != InvalidAIActionGroupId)
		{
			const size_t groupIndex =
				static_cast<size_t>(action.groupId - 1u);

			if (groupIndex >= runtime->groupCooldownSec.size())
			{
				runtime->groupCooldownSec.resize(
					groupIndex + 1u,
					0.0f);
			}

			runtime->groupCooldownSec[groupIndex] =
				action.groupCooldownSec;
		}
	}

	// 4. global cooldown / movement lock 반영
	{
		runtime->globalActionCooldownSec	= action.globalCooldownSec;
		runtime->movementLockSec			= action.lockMovementSec;
	}

	// 5. 마지막 사용 ability / sequence 갱신
	{
		runtime->lastUsedAbilityId = action.abilityId;
		if (action.actionRole == AIActionRole::Basic)
		{
			runtime->basicActionCountSinceEffect =
				static_cast<uint16_t>(std::min<uint32_t>(
					static_cast<uint32_t>(
						runtime->basicActionCountSinceEffect) + 1u,
					std::numeric_limits<uint16_t>::max()));
		}
		else if (action.resetsBasicActionCount)
		{
			runtime->basicActionCountSinceEffect = 0;
		}
		++runtime->actionSequence;
	}
}
