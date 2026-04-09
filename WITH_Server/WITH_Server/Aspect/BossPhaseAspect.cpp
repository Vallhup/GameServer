#include "pch.h"
#include "BossPhaseAspect.h"

CharacterFeatureFlags BossPhaseAspect::RequiredFeature() const noexcept
{
	return CharacterFeatureFlags::BossPhase;
}

void BossPhaseAspect::RegisterStorages(WorldRuntime& runtime) const
{
	(void)runtime;
}

void BossPhaseAspect::Attach(
	WorldRuntime& runtime,
	Entity entity,
	const CharacterDef& def,
	const AssembleParams& params) const
{
	(void)runtime;
	(void)entity;
	(void)def;
	(void)params;
}
