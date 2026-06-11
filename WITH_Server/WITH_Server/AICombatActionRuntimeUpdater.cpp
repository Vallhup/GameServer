#include "pch.h"
#include "AICombatActionRuntimeUpdater.h"

#include "AIBehaviorDef.h"
#include "IAIState.h"
#include "System.h"

void AICombatActionRuntimeUpdater::Commit(
	AIContext& ctx,
	const CombatActionSelection& selection)
{
	if (!selection.shouldAttack ||
		selection.selectedActionIndex ==
			CombatActionSelection::InvalidActionIndex ||
		ctx.behaviorProfile == nullptr ||
		selection.selectedActionIndex >=
			ctx.behaviorProfile->combatActionDefs.size())
	{
		return;
	}

	const AIActionDef& action =
		ctx.behaviorProfile->combatActionDefs[
			selection.selectedActionIndex];
	if (action.abilityId != selection.selectedAbilityId)
		return;

	ApplyAction(ctx, action, selection.selectedActionIndex);
}

void AICombatActionRuntimeUpdater::ApplyAction(
	AIContext& ctx,
	const AIActionDef& action,
	size_t actionIndex)
{
	AIActionRuntimeComp* runtime = ctx.actionRuntime;
	if (runtime == nullptr && ctx.sysCtx != nullptr)
	{
		runtime =
			ctx.sysCtx->ecs.GetMutableComponent<AIActionRuntimeComp>(
				ctx.self);
	}
	if (runtime == nullptr)
		return;

	if (actionIndex >= runtime->actionCooldownSec.size())
	{
		runtime->actionCooldownSec.resize(actionIndex + 1u, 0.0f);
	}
	runtime->actionCooldownSec[actionIndex] = action.aiCooldownSec;

	if (action.groupId != InvalidAIActionGroupId)
	{
		const size_t groupIndex =
			static_cast<size_t>(action.groupId - 1u);
		if (groupIndex >= runtime->groupCooldownSec.size())
		{
			runtime->groupCooldownSec.resize(groupIndex + 1u, 0.0f);
		}
		runtime->groupCooldownSec[groupIndex] =
			action.groupCooldownSec;
	}

	runtime->globalActionCooldownSec = action.globalCooldownSec;
	runtime->movementLockSec = action.lockMovementSec;
	runtime->lastUsedAbilityId = action.abilityId;

	if (action.actionRole == AIActionRole::Basic)
	{
		runtime->basicActionCountSinceEffect =
			static_cast<uint16_t>(std::min<uint32_t>(
				static_cast<uint32_t>(
					runtime->basicActionCountSinceEffect) + 1u,
				std::numeric_limits<uint16_t>::max()));
	}
	else if (action.resetsBasicActionCount)
	{
		runtime->basicActionCountSinceEffect = 0;
	}

	++runtime->actionSequence;
}
