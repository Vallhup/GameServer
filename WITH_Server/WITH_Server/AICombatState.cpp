#include "pch.h"
#include "AICombatState.h"

#include "ECS/GameplayRuntimeComponents.h"
#include "ECS/System/AbilityProfileService.h"
#include "IAIMovementPolicy.h"
#include "IAICombatActionPolicy.h"
#include "System.h"

void AICombatState::Enter(AIContext& ctx) const
{
	if (ctx.command)
		ctx.command->ClearAll();
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

	if (ctx.abilityState == nullptr || !ctx.abilityState->CanIssueAbility())
		return;

	if (ctx.combatActionPolicy == nullptr)
		return;

	const CombatActionSelection selection = ctx.combatActionPolicy->SelectAction(ctx);
	if (!selection.shouldAttack)
		return;

	if (!IsAbilityAvailableForSelf(ctx, selection.selectedAbilityId))
		return;

	ctx.command->hasAbility  = true;
	ctx.command->abilityId   = selection.selectedAbilityId;
	ctx.command->abilityDirX = selection.directionX;
	ctx.command->abilityDirZ = selection.directionZ;
	ctx.command->sequence++;

	ctx.blackboard->lastUsedAbilityId = selection.selectedAbilityId;
	ctx.blackboard->combatActionSequence++;
	ctx.decision->attackCooldownAcc  = 0.0;
}

void AICombatState::FrameUpdate(AIContext& ctx, const double dT) const
{
	ctx.command->hasLook = true;
	ctx.command->target  = ctx.blackboard->currentTarget;

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
