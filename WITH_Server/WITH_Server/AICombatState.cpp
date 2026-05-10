#include "pch.h"
#include "AICombatState.h"

#include "ECS/GameplayRuntimeComponents.h"
#include "ECS/System/AbilityProfileService.h"
#include "IAIMovementPolicy.h"
#include "IAICombatActionPolicy.h"
#include "System.h"

void AICombatState::Enter(AIContext& ctx) const
{
	if (ctx.intent)
		ctx.intent->ClearAll();
}

void AICombatState::DecisionUpdate(AIContext& ctx, const double decisionDT) const
{
	if (ctx.blackboard->returningHome)
	{
		ctx.decision->RequestTransition(AIStateType::ReturnHome);
		return;
	}

	if (!ctx.perception->hasTarget)
	{
		ctx.decision->RequestTransition(AIStateType::Search);
		return;
	}

	if (!ctx.perception->targetInAttackRange)
	{
		ctx.decision->RequestTransition(AIStateType::Chase);
		return;
	}


	if (ctx.abilityState->CanIssueAbility())
	{
		const CombatActionSelection selection = ctx.combatActionPolicy->SelectAction(ctx);
		if (selection.shouldAttack)
		{
			if (IsAbilityAvailableForSelf(ctx, selection.selectedAbilityId))
			{
				ctx.intent->hasAbility = true;
				ctx.intent->abilityId = selection.selectedAbilityId;
				ctx.intent->abilityTarget = selection.target;
				ctx.intent->abilityDirX = selection.directionX;
				ctx.intent->abilityDirZ = selection.directionZ;
				ctx.intent->sequence++;

				ctx.actionRuntime->lastUsedAbilityId = selection.selectedAbilityId;
				ctx.actionRuntime->actionSequence++;
			}
		}
	}
}

void AICombatState::FrameUpdate(AIContext& ctx, const double dT) const
{
	ctx.intent->hasLook = true;
	ctx.intent->target  = ctx.blackboard->currentTarget;

	ctx.movementPolicy->BuildCombatIntent(ctx);
}

bool AICombatState::IsAbilityAvailableForSelf(const AIContext& ctx, AbilityId abilityId) noexcept
{
	if (abilityId == InvalidAbilityId || ctx.sysCtx == nullptr)
	{
		return false;
	}

	const SpawnTypeComp* spawnType = ctx.sysCtx->ecs.GetComponent<SpawnTypeComp>(ctx.self);
	if (spawnType == nullptr)
	{
		return false;
	}

	return AbilityProfileService::IsAbilityAvailable(spawnType->characterId, abilityId);
}
