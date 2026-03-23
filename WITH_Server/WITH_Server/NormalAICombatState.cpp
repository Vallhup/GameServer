#include "pch.h"
#include "NormalAICombatState.h"

#include "Action.h"
#include "IMovementPolicy.h"

void NormalAICombatState::Enter(AIContext& ctx) const
{
	if (ctx.intent)
		ctx.intent->ClearAll();
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

	if (!ctx.actionState->CanIssueDecision()) return;

	if (ctx.decision->attackCooldownAcc >= ctx.decisionTuning->attackCooldown)
	{
		// TEMP : AttackType 결정 별도 인터페이스 훅으로 주입
		ctx.intent->action.hasAction = true;
		ctx.intent->action.actionType = ActionType::Attack;
		ctx.intent->action.attackType = AttackType::Slash;
		ctx.intent->action.sequence++;

		ctx.decision->attackCooldownAcc = 0.0;
		return;
	}
}

void NormalAICombatState::FrameUpdate(AIContext& ctx, const double dT) const
{
	ctx.intent->motion.hasLookTarget = true;
	ctx.intent->motion.lookTarget = ctx.blackboard->currentTarget;

	ctx.movementPolicy->BuildCombatIntent(ctx);
}
