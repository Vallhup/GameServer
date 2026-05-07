#include "pch.h"
#include "BossPhaseAspect.h"

#include "../ECS/GameplayRuntimeComponents.h"
#include "WorldRuntime.h"

CharacterFeatureFlags BossPhaseAspect::RequiredFeature() const noexcept
{
	return CharacterFeatureFlags::BossPhase;
}

void BossPhaseAspect::RegisterStorages(WorldRuntime& runtime) const
{
	runtime.RegisterStorage<AIPhaseRuntimeComp>();
}

void BossPhaseAspect::Attach(
	WorldRuntime& runtime,
	Entity entity,
	const CharacterDef& def,
	const AssembleParams& params) const
{
	(void)def;
	(void)params;

	AIPhaseRuntimeComp phase{};
	phase.currentPhase = 1;
	phase.crossedThresholdMask = 0;
	phase.transitionRequested = false;
	phase.pendingTransitionIndex =
		AIPhaseRuntimeComp::kInvalidTransitionIndex;
	runtime.DeferredUpsertComponent<AIPhaseRuntimeComp>(entity, phase);
}

bool BossPhaseAspect::Validate(
	const CharacterDef& def,
	std::string& outError) const
{
	if (def.role != CharacterRole::Boss)
	{
		outError = "BossPhase feature requires role == Boss";
		return false;
	}
	return true;
}
