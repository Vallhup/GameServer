#include "pch.h"
#include "AICombatState.h"

#include "AICombatRangePolicy.h"
#include "AITargetAttackTracker.h"
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
	if (ShouldReturnHome(ctx))
	{
		ctx.decision->RequestTransition(AIStateType::ReturnHome);
		return;
	}

	if (ShouldSearchForTarget(ctx))
	{
		ctx.decision->RequestTransition(AIStateType::Search);
		return;
	}

	if (ShouldResumeChase(ctx))
	{
		ctx.decision->RequestTransition(AIStateType::Chase);
		return;
	}

	TryIssueCombatAction(ctx);
}

void AICombatState::FrameUpdate(AIContext& ctx, const double dT) const
{
	ctx.intent->hasLook = true;
	ctx.intent->target  = ctx.blackboard->currentTarget;

	ctx.movementPolicy->BuildCombatIntent(ctx);
}

bool AICombatState::ShouldReturnHome(const AIContext& ctx) noexcept
{
	return
		ctx.blackboard != nullptr &&
		ctx.blackboard->returningHome;
}

bool AICombatState::ShouldSearchForTarget(const AIContext& ctx) noexcept
{
	return
		ctx.perception == nullptr ||
		!ctx.perception->hasTarget;
}

bool AICombatState::ShouldResumeChase(const AIContext& ctx) noexcept
{
	return AICombatRangePolicy::ShouldExitCombat(ctx);
}

void AICombatState::TryIssueCombatAction(AIContext& ctx)
{
	if (ctx.abilityState == nullptr ||
		!ctx.abilityState->CanIssueAbility() ||
		ctx.combatActionPolicy == nullptr ||
		ctx.intent == nullptr)
	{
		return;
	}

	const CombatActionSelection selection =
		ctx.combatActionPolicy->SelectAction(ctx);
	if (!selection.shouldAttack ||
		!IsAbilityAvailableForSelf(ctx, selection.selectedAbilityId))
	{
		return;
	}

	ctx.intent->hasAbility = true;
	ctx.intent->abilityId = selection.selectedAbilityId;
	ctx.intent->abilityTarget = selection.target;
	ctx.intent->abilityDirX = selection.directionX;
	ctx.intent->abilityDirZ = selection.directionZ;
	++ctx.intent->sequence;

	ctx.combatActionPolicy->CommitSelection(ctx, selection);
	AITargetAttackTracker::RecordCommittedAttack(
		ctx,
		selection.target);
}

bool AICombatState::IsAbilityAvailableForSelf(const AIContext& ctx, AbilityId abilityId) noexcept
{
	if (abilityId == InvalidAbilityId || 
		ctx.sysCtx == nullptr)
	{
		return false;
	}

	if (const SpawnTypeComp* spawnType =
		ctx.sysCtx->ecs.GetComponent<SpawnTypeComp>(ctx.self))
	{
		return AbilityProfileService::IsAbilityAvailable(spawnType->characterId, abilityId);
	}

	return false;
}
