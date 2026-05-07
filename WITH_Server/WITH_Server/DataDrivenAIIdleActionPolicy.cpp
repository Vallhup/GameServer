#include "pch.h"
#include "DataDrivenAIIdleActionPolicy.h"

#include "AICombatActionPolicyUtil.h"
#include "ECS/System/AbilityProfileService.h"
#include "ECS/Components/GameplayAIComponents.h"

void DataDrivenAIIdleActionPolicy::TryIssueIdleAction(AIContext& ctx) const
{
	if (!CanStartIdleAction(ctx))
	{
		return;
	}

	if (ctx.actionRuntime->idleActionCooldownAcc <
		ctx.perceptionTuning->idleActionCooldownSec)
	{
		return;
	}

	IdleActionSelectionContext selection = BuildSelectionContext(ctx);

	ctx.actionRuntime->idleActionCooldownAcc = 0.0;

	const int chanceRoll =
		AICombatActionPolicyUtil::PseudoRand(ctx.self.id, selection.sequence, 100);

	if (chanceRoll >= ctx.perceptionTuning->idleActionChancePercent)
	{
		return;
	}

	std::vector<IdleActionCandidate> candidates =
		CollectCandidates(selection);

	size_t selectedActionIndex{ 0 };
	const AIActionDef* selectedAction =
		PickIdleAction(selection, candidates, selectedActionIndex);

	if (selectedAction == nullptr)
		return;

	IssueIdleAction(selection, *selectedAction);
	ApplyRuntimeSelection(
		selection,
		*selectedAction,
		static_cast<size_t>(selection.runtimeOffset) +
		selectedActionIndex);
}
bool DataDrivenAIIdleActionPolicy::CanStartIdleAction(AIContext& ctx) noexcept
{
    return 
		ctx.sysCtx				!= nullptr &&
		ctx.behaviorProfile		!= nullptr &&
		ctx.intent				!= nullptr &&
		ctx.abilityState		!= nullptr &&
		ctx.perceptionTuning	!= nullptr &&
		ctx.actionRuntime		!= nullptr &&
		!ctx.behaviorProfile->idleActionDefs.empty() &&
		ctx.abilityState->CanIssueAbility();
}

DataDrivenAIIdleActionPolicy::IdleActionSelectionContext 
DataDrivenAIIdleActionPolicy::BuildSelectionContext(AIContext& ctx) noexcept
{
	uint8_t phase{ 1 };
	float selfHpRatio{ 1.0f };

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

	// 2. selfHpRatio 계산
	{
		const CombatStatStateComp* statComp = ctx.stats;

		if (statComp != nullptr)
		{
			if (statComp->maxHp > 0)
			{
				const float rawHpRatio =
					static_cast<float>(statComp->currentHp) / 
					static_cast<float>(statComp->maxHp);

				selfHpRatio = std::clamp(rawHpRatio, 0.0f, 1.0f);
			}
		}
	}

	return IdleActionSelectionContext
	{
		.aiCtx				= ctx,
		.runtime			= *ctx.actionRuntime,
		.actions			= ctx.behaviorProfile->idleActionDefs,
		.runtimeOffset		= ctx.behaviorProfile->combatActionDefs.size(),
		.sequence			= ctx.actionRuntime->idleActionSequence++,
		.phase				= phase,
		.selfHpRatio		= selfHpRatio,
		.lastUsedAbilityId	= ctx.actionRuntime->lastUsedAbilityId
	};
}

std::vector<DataDrivenAIIdleActionPolicy::IdleActionCandidate> 
DataDrivenAIIdleActionPolicy::CollectCandidates(
	const IdleActionSelectionContext& selection)
{
	std::vector<IdleActionCandidate> candidates;
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

		// 2. self HP ratio 조건 검사
		{
			if (selection.selfHpRatio < condition.selfHpRatioMin ||
				selection.selfHpRatio > condition.selfHpRatioMax)
			{
				continue;
			}
		}

		// 3. target 관련 조건 검사
		{
			if (condition.distanceBucket != AIActionDistanceBucket::Any)
			{
				if (selection.aiCtx.perception == nullptr ||
					!selection.aiCtx.perception->hasTarget)
				{
					continue;
				}
			}

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

		// 4. ability 사용 가능 조건 검사
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

		const size_t runtimeIndex =
			static_cast<size_t>(selection.runtimeOffset + i);

		// 5. action cooldown 검사
		{
			if (runtimeIndex < selection.runtime.actionCooldownSec.size() &&
				selection.runtime.actionCooldownSec[runtimeIndex] > 0.0f)
			{
				continue;
			}
		}

		// 6. group cooldown 검사
		{
			if (action.groupId != InvalidAIActionGroupId)
			{
				const size_t groupIndex =
					static_cast<size_t>(action.groupId - 1u);

				if (groupIndex <
					selection.runtime.groupCooldownSec.size() &&
					selection.runtime.groupCooldownSec[groupIndex] > 0.0f)
				{
					continue;
				}
			}
		}

		// 7. weight 계산
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

		candidates.push_back(IdleActionCandidate
			{
				.actionIndex = i,
				.weight = weight
			});
	}

	return candidates;
}

const AIActionDef* DataDrivenAIIdleActionPolicy::PickIdleAction(
	const IdleActionSelectionContext& selection, 
	std::span<const IdleActionCandidate> candidates, 
	size_t& outActionIndex) noexcept
{
	outActionIndex = 0;

	if (candidates.empty())
		return nullptr;

	uint32_t totalWeight{ 0 };

	// 1. total weight 계산
	{
		for (const IdleActionCandidate& candidate : candidates)
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
		constexpr uint32_t kIdleActionPickSalt = 0x85ebca6bu;

		cursor =
			static_cast<uint32_t>(
				AICombatActionPolicyUtil::PseudoRand(
					selection.aiCtx.self.id,
					selection.sequence + kIdleActionPickSalt,
					static_cast<int>(totalWeight)));
	}

	// 3. cursor 위치에 해당하는 candidate 선택
	{
		for (const IdleActionCandidate& candidate : candidates)
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

void DataDrivenAIIdleActionPolicy::IssueIdleAction(
	const IdleActionSelectionContext& selection, 
	const AIActionDef& action) noexcept
{
	if (selection.aiCtx.intent == nullptr)
		return;

	// 1. ability command 작성
	{
		selection.aiCtx.intent->hasAbility = true;
		selection.aiCtx.intent->abilityId = action.abilityId;

		selection.aiCtx.intent->abilityDirX = 0.0f;
		selection.aiCtx.intent->abilityDirZ = 0.0f;

		selection.aiCtx.intent->sequence++;
	}
}

void DataDrivenAIIdleActionPolicy::ApplyRuntimeSelection(
	const IdleActionSelectionContext& selection, 
	const AIActionDef& action, 
	size_t runtimeIndex)
{
	AIActionRuntimeComp& runtime = selection.runtime;

	// 1. action cooldown slot 보장
	{
		if (runtimeIndex >= runtime.actionCooldownSec.size())
		{
			runtime.actionCooldownSec.resize(
				runtimeIndex + 1u,
				0.0f);
		}
	}

	// 2. action cooldown 반영
	{
		runtime.actionCooldownSec[runtimeIndex] =
			action.aiCooldownSec;
	}

	// 3. group cooldown slot 보장 및 반영
	{
		if (action.groupId != InvalidAIActionGroupId)
		{
			const size_t groupIndex =
				static_cast<size_t>(action.groupId - 1u);

			if (groupIndex >= runtime.groupCooldownSec.size())
			{
				runtime.groupCooldownSec.resize(
					groupIndex + 1u,
					0.0f);
			}

			runtime.groupCooldownSec[groupIndex] =
				action.groupCooldownSec;
		}
	}

	// 4. global cooldown / movement lock 반영
	{
		runtime.globalActionCooldownSec = std::max(
			runtime.globalActionCooldownSec,
			action.globalCooldownSec);

		runtime.movementLockSec = std::max(
			runtime.movementLockSec,
			action.lockMovementSec);
	}

	// 5. 마지막 사용 ability / sequence 갱신
	{
		runtime.lastUsedAbilityId = action.abilityId;
		++runtime.actionSequence;
	}
}
