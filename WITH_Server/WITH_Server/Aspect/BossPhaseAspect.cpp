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
	runtime.RegisterStorage<BossGimmickStateComp>();
	runtime.RegisterStorage<GimmickObjectComp>();
	runtime.RegisterStorage<StaticBoxHurtColliderComp>();
	runtime.RegisterStorage<SafeZoneComp>();
	runtime.RegisterStorage<BossGimmickImmunityComp>();
	runtime.RegisterStorage<PendingBossGimmickReplicationComp>();
}

void BossPhaseAspect::Attach(
	WorldRuntime& runtime,
	Entity entity,
	const CharacterDef& def,
	const AssembleParams& params) const
{
	runtime.DeferredUpsertComponent<AIPhaseRuntimeComp>(entity,
		AIPhaseRuntimeComp
		{
			.currentPhase = 1,
			.crossedThresholdMask = 0,
			.transitionRequested = false,
			.pendingTransitionIndex = AIPhaseRuntimeComp::kInvalidTransitionIndex
		});

	//runtime.DeferredUpsertComponent<BossGimmickStateComp>(
	//	entity,
	//	BossGimmickStateComp{});

	runtime.DeferredUpsertComponent<PendingBossGimmickReplicationComp>(
		entity,
		PendingBossGimmickReplicationComp{});
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
