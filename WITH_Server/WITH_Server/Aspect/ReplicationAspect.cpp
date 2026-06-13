#include "pch.h"
#include "ReplicationAspect.h"

#include "../ECS/GameplayRuntimeComponents.h"
#include "RepComponent.h"
#include "WorldRuntime.h"

CharacterFeatureFlags ReplicationAspect::RequiredFeature() const noexcept
{
	return CharacterFeatureFlags::Replicated;
}

void ReplicationAspect::RegisterStorages(WorldRuntime& runtime) const
{
	runtime.RegisterStorage<ReplicatedTag>();
	runtime.RegisterStorage<DirtyFlagsComp>();
	runtime.RegisterStorage<ReplicationStatsComp>();
	runtime.RegisterStorage<GameplayEffectReplicationComp>();
}

void ReplicationAspect::Attach(
	WorldRuntime& runtime,
	Entity entity,
	const CharacterDef& def,
	const AssembleParams& params) const
{
	runtime.DeferredAddComponent<ReplicatedTag>(entity);
	runtime.DeferredAddComponent<DirtyFlagsComp>(entity);
	runtime.DeferredAddComponent<ReplicationStatsComp>(entity);
	runtime.DeferredAddComponent<GameplayEffectReplicationComp>(entity);
}
