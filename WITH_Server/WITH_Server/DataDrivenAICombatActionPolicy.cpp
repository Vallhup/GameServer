#include "pch.h"
#include "DataDrivenAICombatActionPolicy.h"

#include "AICombatActionContextBuilder.h"
#include "AICombatActionPolicyUtil.h"
#include "AICombatActionRuntimeUpdater.h"
#include "AICombatActionSelector.h"

CombatActionSelection DataDrivenAICombatActionPolicy::SelectAction(
	const AIContext& ctx) const
{
	CombatActionSelection result{};
	if (!AICombatActionContextBuilder::CanBuild(ctx))
		return result;

	const AICombatActionSelectionContext selectionContext =
		AICombatActionContextBuilder::Build(ctx);
	const AICombatActionChoice choice =
		AICombatActionSelector::Select(selectionContext);
	if (!choice.IsValid(selectionContext.actions.size()))
		return result;

	const AIActionDef& action =
		selectionContext.actions[choice.actionIndex];

	result.shouldAttack = true;
	result.selectedActionIndex = choice.actionIndex;
	result.selectedAbilityId = action.abilityId;
	result.target = ctx.perception->selectedTarget;
	AICombatActionPolicyUtil::FillAttackDirectionTowardTarget(ctx, result);
	return result;
}

void DataDrivenAICombatActionPolicy::CommitSelection(
	AIContext& ctx,
	const CombatActionSelection& selection) const
{
	AICombatActionRuntimeUpdater::Commit(ctx, selection);
}
