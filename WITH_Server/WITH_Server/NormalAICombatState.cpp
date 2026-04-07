#include "pch.h"
#include "NormalAICombatState.h"

#include "ECS/GameplayRuntimeComponents.h"
#include "IAIMovementPolicy.h"

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

	if (!ctx.actionState->CanIssueAction()) return;

	if (ctx.decision->attackCooldownAcc >= ctx.decisionTuning->attackCooldown)
	{
		// TEMP : AttackType 결정 별도 인터페이스 훅으로 주입
		ctx.command->hasAction = true;
		ctx.command->actionId = ActionId::Imp_melee1;
		ctx.command->actionDirX = ctx.selfTr->rotation.x;
		ctx.command->actionDirZ = ctx.selfTr->rotation.z;
		ctx.command->sequence++;

		ctx.decision->attackCooldownAcc = 0.0;
		return;
	}
}

void NormalAICombatState::FrameUpdate(AIContext& ctx, const double dT) const
{
	ctx.command->hasLook = true;
	ctx.command->target = ctx.blackboard->currentTarget;

	ctx.movementPolicy->BuildCombatIntent(ctx);
}
