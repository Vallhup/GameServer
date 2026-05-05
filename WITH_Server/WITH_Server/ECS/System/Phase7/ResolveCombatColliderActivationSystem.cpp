#include "pch.h"
#include "ResolveCombatColliderActivationSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 2> kResolveCombatColliderActivationAccesses{
		ReadImmediate(ComponentRes<AbilityStateComp>()),
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
	for (auto [entity, abilityState, activation] :
		ctx.ecs.View<AbilityStateComp, CombatColliderActivationComp>())
	{
		if (HasBlockingPendingState(ctx.ecs, entity) ||
			!IsAbilityActive(abilityState))
		{
			activation = {};
			continue;
		}

		const AbilityDef* abilityDef =
			GameplayContentCatalogSnapshot::Current().Abilities().Find(abilityState.abilityId);
		const float normalizedTime =
			(abilityDef != nullptr && abilityDef->timeline.durationSec > 0.0f)
			? ClampFloat(abilityState.elapsedSec / abilityDef->timeline.durationSec, 0.0f, 1.0f)
			: 0.0f;

		activation.boundAbilityInstanceId = abilityState.abilityInstanceId;
		activation.boundAbilityId = abilityState.abilityId;
		activation.hasAttackWindow =
			abilityDef != nullptr &&
			IsWindowActive(*abilityDef, AbilityCombatWindowKind::Attack, normalizedTime);
		//printf("[ResolveCombatColliderActivationSystem] hasAttackWindow: %d\n", activation.hasAttackWindow);
		activation.hasParryWindow =
			abilityDef != nullptr &&
			IsWindowActive(*abilityDef, AbilityCombatWindowKind::Parry, normalizedTime);
		activation.hasGuardWindow =
			abilityDef != nullptr &&
			IsWindowActive(*abilityDef, AbilityCombatWindowKind::Guard, normalizedTime);
		activation.hasInvulnerabilityWindow =
			abilityDef != nullptr &&
			IsWindowActive(
				*abilityDef,
				AbilityCombatWindowKind::Invulnerability,
				normalizedTime);
	}
}

const SystemMeta& ResolveCombatColliderActivationSystem::Meta() const
{
	return kMeta;
}
