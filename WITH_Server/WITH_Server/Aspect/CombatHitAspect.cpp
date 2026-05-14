#include "pch.h"
#include "CombatHitAspect.h"

#include "../ECS/GameplayRuntimeComponents.h"
#include "WorldRuntime.h"

CharacterFeatureFlags CombatHitAspect::RequiredFeature() const noexcept
{
	return CharacterFeatureFlags::Combatant;
}

void CombatHitAspect::RegisterStorages(WorldRuntime& runtime) const
{
	runtime.RegisterStorage<CombatColliderActivationComp>();
	runtime.RegisterStorage<CombatHitDedupStateComp>();
	runtime.RegisterStorage<PendingCombatResultComp>();
	runtime.RegisterStorage<PendingCombatImpactEventComp>();
}

void CombatHitAspect::Attach(
	WorldRuntime& runtime,
	Entity entity,
	const CharacterDef& def,
	const AssembleParams& params) const
{
	runtime.DeferredAddComponent<CombatColliderActivationComp>(entity);
	runtime.DeferredAddComponent<CombatHitDedupStateComp>(entity);
	runtime.DeferredAddComponent<PendingCombatResultComp>(entity);
	runtime.DeferredAddComponent<PendingCombatImpactEventComp>(entity);
}
