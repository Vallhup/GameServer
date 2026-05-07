#include "pch.h"
#include "DataDrivenAISpecialActionPolicy.h"

#include "AIBehaviorDef.h"
#include "ECS/System/AbilityProfileService.h"
#include "ECS/System/GameplaySystemUtil.h"
#include "IAIState.h"
#include "System.h"

#include <algorithm>

namespace
{
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

	const AIPhaseTransitionDef* ResolvePendingTransition(
		const AIContext& ctx,
		const AIPhaseRuntimeComp& phase) noexcept
	{
		if (ctx.behaviorProfile == nullptr)
			return nullptr;

		const std::span<const AIPhaseTransitionDef> transitions =
			ctx.behaviorProfile->phaseTransitions;
		if (transitions.empty())
			return nullptr;

		if (phase.pendingTransitionIndex !=
			AIPhaseRuntimeComp::kInvalidTransitionIndex)
		{
			const size_t index =
				static_cast<size_t>(phase.pendingTransitionIndex);
			if (index < transitions.size())
				return &transitions[index];
		}

		for (const AIPhaseTransitionDef& transition : transitions)
		{
			if (transition.toPhase == phase.currentPhase)
				return &transition;
		}

		return nullptr;
	}

	void ClearPendingTransition(AIPhaseRuntimeComp& phase) noexcept
	{
		phase.transitionRequested = false;
		phase.pendingTransitionIndex =
			AIPhaseRuntimeComp::kInvalidTransitionIndex;
	}

	void ApplyTransitionRuntimeEffects(
		AIContext& ctx,
		const AIPhaseTransitionDef& transition) noexcept
	{
		AIActionRuntimeComp* actionRuntime = ctx.actionRuntime;
		if (actionRuntime == nullptr && ctx.sysCtx != nullptr)
		{
			actionRuntime =
				ctx.sysCtx->ecs.GetMutableComponent<AIActionRuntimeComp>(
					ctx.self);
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

			actionRuntime->globalActionCooldownSec =
				std::max(
					actionRuntime->globalActionCooldownSec,
					transition.transitionLockSec);
			actionRuntime->movementLockSec =
				std::max(
					actionRuntime->movementLockSec,
					transition.transitionLockSec);
		}

		AIMovementRuntimeComp* movementRuntime = ctx.movementRuntime;
		if (movementRuntime == nullptr && ctx.sysCtx != nullptr)
		{
			movementRuntime =
				ctx.sysCtx->ecs.GetMutableComponent<AIMovementRuntimeComp>(
					ctx.self);
		}
		if (movementRuntime != nullptr)
		{
			movementRuntime->strafeTimeLeftSec = 0.0f;
		}
	}

	bool TryFillDirectionToCurrentTarget(
		AIContext& ctx,
		float& outX,
		float& outZ) noexcept
	{
		using namespace GameplaySystemUtil;

		outX = 0.0f;
		outZ = 0.0f;

		if (ctx.sysCtx == nullptr ||
			ctx.blackboard == nullptr ||
			ctx.blackboard->currentTarget.IsNull() ||
			ctx.selfTr == nullptr)
		{
			return false;
		}

		const WorldTransformComp* targetTr =
			ctx.sysCtx->ecs.GetComponent<WorldTransformComp>(
				ctx.blackboard->currentTarget);
		if (targetTr == nullptr)
			return false;

		outX = targetTr->position.x - ctx.selfTr->position.x;
		outZ = targetTr->position.z - ctx.selfTr->position.z;

		NormalizeXZ(outX, outZ);
		return LengthXZ(outX, outZ) > kOverlapEpsilon;
	}
}

void DataDrivenAISpecialActionPolicy::TickRuntime(
	AIContext& ctx,
	const double dtSec) const
{
	(void)ctx;
	(void)dtSec;
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

	if (ctx.intent == nullptr ||
		ctx.abilityState == nullptr ||
		!ctx.abilityState->CanIssueAbility())
	{
		return false;
	}

	float dirX{ 0.0f };
	float dirZ{ 0.0f };
	TryFillDirectionToCurrentTarget(ctx, dirX, dirZ);

	ctx.intent->ClearAll();
	ctx.intent->hasLook = true;
	ctx.intent->target =
		ctx.blackboard != nullptr
		? ctx.blackboard->currentTarget
		: Entity::Null();
	ctx.intent->hasAbility = true;
	ctx.intent->abilityId = transition->transitionAbilityId;
	ctx.intent->abilityDirX = dirX;
	ctx.intent->abilityDirZ = dirZ;
	ctx.intent->sequence++;

	ApplyTransitionRuntimeEffects(ctx, *transition);
	if (transition->forceRetarget && ctx.blackboard != nullptr)
	{
		ctx.blackboard->forceRetarget = true;
	}

	ClearPendingTransition(*phase);
	return true;
}
