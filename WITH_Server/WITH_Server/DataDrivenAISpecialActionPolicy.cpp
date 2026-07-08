#include "pch.h"
#include "DataDrivenAISpecialActionPolicy.h"

#include "ECS/Components/GameplayAIComponents.h"
#include "ECS/System/AbilityProfileService.h"
#include "ECS/System/GameplaySystemUtil.h"
#include "System.h"

void DataDrivenAISpecialActionPolicy::TickRuntime(
	AIContext& ctx,
	const double dtSec) const
{
	if (ctx.sysCtx == nullptr)
		return;

	AIPhaseRuntimeComp* phase =
		ctx.sysCtx->ecs.GetMutableComponent<AIPhaseRuntimeComp>(ctx.self);
	if (phase == nullptr ||
		phase->transitionInvulnerabilityRemainingSec <= 0.0f)
	{
		return;
	}

	phase->transitionInvulnerabilityRemainingSec = std::max(
		0.0f,
		phase->transitionInvulnerabilityRemainingSec -
			static_cast<float>(std::max(0.0, dtSec)));
}

bool DataDrivenAISpecialActionPolicy::TryIssuePreFSMAction(AIContext& ctx) const
{
	if (ctx.sysCtx == nullptr)
		return false;

	AIPhaseRuntimeComp* phase =
		ctx.sysCtx->ecs.GetMutableComponent<AIPhaseRuntimeComp>(ctx.self);

	if (phase == nullptr || !phase->transitionRequested)
		return false;

	const AIPhaseTransitionDef* transition =
		ResolvePendingTransition(ctx, *phase);

	// 1. transition / ability 유효성 검사 (실패 시 pending 정리)
	{
		if (transition == nullptr ||
			transition->transitionAbilityId == InvalidAbilityId)
		{
			ClearPendingTransition(*phase);
			return false;
		}

		if (!IsAbilityAvailableForSelf(ctx, transition->transitionAbilityId))
		{
			ClearPendingTransition(*phase);
			return false;
		}
	}

	// 2. ability 발동 가능 여부 검사 (실패 시 pending 유지)
	{
		if (ctx.intent == nullptr ||
			ctx.abilityState == nullptr ||
			!ctx.abilityState->CanIssueAbility())
		{
			return false;
		}
	}

	// 3. transition ability 발동
	{
		IssueTransitionAbility(ctx, *transition);
	}

	// 4. runtime 상태 반영 및 retarget 처리
	{
		ApplyTransitionRuntimeEffects(ctx, *transition);

		if (transition->forceRetarget && ctx.blackboard != nullptr)
		{
			ctx.blackboard->forceRetarget = true;
		}
	}

	ClearPendingTransition(*phase);
	return true;
}

const AIPhaseTransitionDef*
DataDrivenAISpecialActionPolicy::ResolvePendingTransition(
	const AIContext& ctx,
	const AIPhaseRuntimeComp& phase) noexcept
{
	if (ctx.behaviorProfile == nullptr)
		return nullptr;

	const std::span<const AIPhaseTransitionDef> transitions =
		ctx.behaviorProfile->phaseTransitions;

	if (transitions.empty())
		return nullptr;

	// 1. pending transition index 우선 사용
	{
		if (phase.pendingTransitionIndex !=
			AIPhaseRuntimeComp::kInvalidTransitionIndex)
		{
			const size_t index =
				static_cast<size_t>(phase.pendingTransitionIndex);

			if (index < transitions.size())
				return &transitions[index];
		}
	}

	// 2. fallback: 현재 phase로 향하는 transition 검색
	{
		for (const AIPhaseTransitionDef& transition : transitions)
		{
			if (transition.toPhase == phase.currentPhase)
				return &transition;
		}
	}

	return nullptr;
}

bool DataDrivenAISpecialActionPolicy::IsAbilityAvailableForSelf(
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

void DataDrivenAISpecialActionPolicy::IssueTransitionAbility(
	AIContext& ctx,
	const AIPhaseTransitionDef& transition)
{
	using namespace GameplaySystemUtil;

	float dirX{ 0.0f };
	float dirZ{ 0.0f };

	// 1. 현재 target 방향 계산
	{
		if (ctx.sysCtx != nullptr &&
			ctx.blackboard != nullptr &&
			!ctx.blackboard->currentTarget.IsNull() &&
			ctx.selfTr != nullptr)
		{
			if (const WorldTransformComp* targetTr =
				ctx.sysCtx->ecs.GetComponent<WorldTransformComp>(
					ctx.blackboard->currentTarget))
			{
				const float rawX =
					targetTr->position.x - ctx.selfTr->position.x;
				const float rawZ =
					targetTr->position.z - ctx.selfTr->position.z;

				float normX = rawX;
				float normZ = rawZ;
				NormalizeXZ(normX, normZ);

				if (LengthXZ(normX, normZ) > kOverlapEpsilon)
				{
					dirX = normX;
					dirZ = normZ;
				}
			}
		}
	}

	// 2. intent 작성
	{
		ctx.intent->ClearAll();

		ctx.intent->hasLook		= true;
		ctx.intent->hasAbility	= true;
		ctx.intent->abilityId	= transition.transitionAbilityId;
		ctx.intent->abilityDirX	= dirX;
		ctx.intent->abilityDirZ	= dirZ;

		ctx.intent->sequence++;

		if (ctx.blackboard)
		{
			ctx.intent->target = ctx.blackboard->currentTarget;
		}
		else
		{
			ctx.intent->target = Entity::Null();
		}
	}
}

void DataDrivenAISpecialActionPolicy::ApplyTransitionRuntimeEffects(
	AIContext& ctx,
	const AIPhaseTransitionDef& transition) noexcept
{
	if (ctx.sysCtx != nullptr)
	{
		if (AIPhaseRuntimeComp* phase =
			ctx.sysCtx->ecs.GetMutableComponent<AIPhaseRuntimeComp>(ctx.self))
		{
			phase->transitionInvulnerabilityRemainingSec = std::max(
				phase->transitionInvulnerabilityRemainingSec,
				transition.transitionLockSec);
		}
	}

	// 1. action runtime 효과 반영
	{
		AIActionRuntimeComp* actionRuntime = ctx.actionRuntime;

		if (actionRuntime == nullptr && ctx.sysCtx != nullptr)
		{
			actionRuntime =
				ctx.sysCtx->ecs.GetMutableComponent<AIActionRuntimeComp>(ctx.self);
		}

		if (actionRuntime != nullptr)
		{
			if (transition.clearActionCooldowns)
			{
				std::fill(
					actionRuntime->actionCooldownSec.begin(),
					actionRuntime->actionCooldownSec.end(),
					0.0f);
			}

			if (transition.clearGroupCooldowns)
			{
				std::fill(
					actionRuntime->groupCooldownSec.begin(),
					actionRuntime->groupCooldownSec.end(),
					0.0f);
			}

			actionRuntime->globalActionCooldownSec = std::max(
				actionRuntime->globalActionCooldownSec,
				transition.transitionLockSec);

			actionRuntime->movementLockSec = std::max(
				actionRuntime->movementLockSec,
				transition.transitionLockSec);
		}
	}

	// 2. movement runtime 초기화
	{
		AIMovementRuntimeComp* movementRuntime = ctx.movementRuntime;

		if (movementRuntime == nullptr && ctx.sysCtx != nullptr)
		{
			movementRuntime =
				ctx.sysCtx->ecs.GetMutableComponent<AIMovementRuntimeComp>(ctx.self);
		}

		if (movementRuntime != nullptr)
		{
			movementRuntime->strafeTimeLeftSec = 0.0f;
		}
	}
}

void DataDrivenAISpecialActionPolicy::ClearPendingTransition(
	AIPhaseRuntimeComp& phase) noexcept
{
	phase.transitionRequested		= false;
	phase.pendingTransitionIndex	= AIPhaseRuntimeComp::kInvalidTransitionIndex;
}
