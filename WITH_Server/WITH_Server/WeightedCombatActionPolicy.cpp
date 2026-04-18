#include "pch.h"
#include "WeightedCombatActionPolicy.h"

#include "AICombatActionPolicyUtil.h"
#include "IAIState.h"

CombatActionSelection WeightedCombatActionPolicy::SelectAction(
	const AIContext& ctx) const
{
	CombatActionSelection result{};

	if (ctx.decision == nullptr ||
		ctx.decisionTuning == nullptr ||
		ctx.blackboard == nullptr ||
		ctx.behaviorProfile == nullptr ||
		ctx.decision->attackCooldownAcc < ctx.decisionTuning->attackCooldown ||
		ctx.behaviorProfile->combatActions.empty())
	{
		return result;
	}

	const int roll = AICombatActionPolicyUtil::PseudoRand(
		ctx.self.id,
		ctx.blackboard->combatActionSequence,
		10000);
	const ActionId selectedActionId =
		AICombatActionPolicyUtil::PickWeightedAction(
			ctx.behaviorProfile->combatActions,
			ctx.blackboard->lastUsedActionId,
			roll);

	if (selectedActionId == ActionId::None)
		return result;

	result.shouldAttack = true;
	result.selectedActionId = selectedActionId;
	AICombatActionPolicyUtil::FillAttackDirectionTowardTarget(ctx, result);
	return result;
}
