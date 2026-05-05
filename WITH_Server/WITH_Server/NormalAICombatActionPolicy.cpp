#include "pch.h"
#include "NormalAICombatActionPolicy.h"

#include "AICombatActionPolicyUtil.h"
#include "AIBehaviorDef.h"
#include "IAIState.h"

CombatActionSelection NormalAICombatActionPolicy::SelectAction(
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
	const AbilityId selectedAbilityId =
		AICombatActionPolicyUtil::PickWeightedAbility(
			ctx.behaviorProfile->combatActions,
			ctx.blackboard->lastUsedAbilityId,
			roll);

	if (selectedAbilityId == InvalidAbilityId)
		return result;

	result.shouldAttack = true;
	result.selectedAbilityId = selectedAbilityId;
	AICombatActionPolicyUtil::FillAttackDirectionTowardTarget(ctx, result);
	return result;
}
