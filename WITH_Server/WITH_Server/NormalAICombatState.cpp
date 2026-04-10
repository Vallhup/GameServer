#include "pch.h"
#include "NormalAICombatState.h"

#include "ECS/GameplayRuntimeComponents.h"
#include "IAIMovementPolicy.h"
#include "IAICombatActionPolicy.h"

void NormalAICombatState::Enter(AIContext& ctx) const
{
	if (ctx.command)
		ctx.command->ClearAll();
}

void NormalAICombatState::DecisionUpdate(AIContext& ctx, const double decisionDT) const
{
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

	if (!ctx.actionState->CanIssueAction())
		return;

	if (!ctx.combatActionPolicy)
		return;

	const CombatActionSelection selection = ctx.combatActionPolicy->SelectAction(ctx);

	if (!selection.shouldAttack)
		return;

	ctx.command->hasAction  = true;
	ctx.command->actionId   = selection.selectedActionId;
	ctx.command->actionDirX = selection.directionX;
	ctx.command->actionDirZ = selection.directionZ;
	ctx.command->sequence++;

	ctx.blackboard->lastUsedActionId = selection.selectedActionId;
	ctx.decision->attackCooldownAcc  = 0.0;
}

void NormalAICombatState::FrameUpdate(AIContext& ctx, const double dT) const
{
	ctx.command->hasLook = true;
	ctx.command->target  = ctx.blackboard->currentTarget;

	ctx.movementPolicy->BuildCombatIntent(ctx);
}
