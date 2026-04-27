#include "pch.h"
#include "ResolveCombatColliderActivationSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 2> kResolveCombatColliderActivationAccesses{
		ReadImmediate(ComponentRes<ActionStateComp>()),
		WriteImmediate(ComponentRes<CombatColliderActivationComp>()),
	};
}

const SystemMeta ResolveCombatColliderActivationSystem::kMeta =
	SystemMeta{
		SysTag<ResolveCombatColliderActivationSystem>(),
		"ResolveCombatColliderActivationSystem",
		kResolveCombatColliderActivationAccesses,
		kNoDeps,
		kNoDeps
	};

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
		//printf("[ResolveCombatColliderActivationSystem] hasAttackWindow: %d\n", activation.hasAttackWindow);
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
