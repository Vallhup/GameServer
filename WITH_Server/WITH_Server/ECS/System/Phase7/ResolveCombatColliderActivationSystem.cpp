#include "pch.h"
#include "ResolveCombatColliderActivationSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta ResolveCombatColliderActivationSystem::kMeta =
	MakeSystemMeta<ResolveCombatColliderActivationSystem>(
		"ResolveCombatColliderActivationSystem");

void ResolveCombatColliderActivationSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, actionState, activation] :
		ctx.ecs.View<ActionStateComp, CombatColliderActivationComp>())
	{
		if (HasBlockingPendingState(ctx.ecs, entity) ||
			!IsActionActive(actionState))
		{
			activation = {};
			continue;
		}

		const ActionDef* actionDef = FindActionDef(actionState.actionId);
		const float normalizedTime =
			(actionDef != nullptr && actionDef->duration > 0.0f)
			? ClampFloat(actionState.elapsedSec / actionDef->duration, 0.0f, 1.0f)
			: 0.0f;

		activation.boundActionInstanceId = actionState.actionInstanceId;
		activation.boundActionId = actionState.actionId;
		activation.hasAttackWindow =
			actionDef != nullptr &&
			IsWindowActive(*actionDef, CombatWindowType::Attack, normalizedTime);
		activation.hasParryWindow =
			actionDef != nullptr &&
			IsWindowActive(*actionDef, CombatWindowType::Parry, normalizedTime);
		activation.hasGuardWindow =
			actionDef != nullptr &&
			IsWindowActive(*actionDef, CombatWindowType::Guard, normalizedTime);
		activation.hasInvulnerabilityWindow =
			actionDef != nullptr &&
			IsWindowActive(
				*actionDef,
				CombatWindowType::Invulnerability,
				normalizedTime);
	}
}

const SystemMeta& ResolveCombatColliderActivationSystem::Meta() const
{
	return kMeta;
}
