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
	runtime.RegisterStorage<BossPhaseStateComp>();
	runtime.RegisterStorage<BossPatternRuntimeComp>();
}

void BossPhaseAspect::Attach(
	WorldRuntime& runtime,
	Entity entity,
	const CharacterDef& def,
	const AssembleParams& params) const
{
	(void)def;
	(void)params;

	BossPhaseStateComp phase{};
	phase.currentPhase = 1;
	phase.phase2ThresholdRatio = 0.55f;
	phase.crossedThresholdMask = 0;
	phase.transitionRequested = false;
	runtime.DeferredUpsertComponent<BossPhaseStateComp>(entity, phase);

	BossPatternRuntimeComp patternRuntime{};
	patternRuntime.patternCooldownSec.fill(0.0f);
	patternRuntime.phaseTransitionLockSec = 0.0f;
	patternRuntime.phaseTransitionActionPending = false;
	patternRuntime.strafeTimeLeftSec = 0.0f;
	patternRuntime.strafeSign = 1;
	runtime.DeferredUpsertComponent<BossPatternRuntimeComp>(
		entity,
		patternRuntime);
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
