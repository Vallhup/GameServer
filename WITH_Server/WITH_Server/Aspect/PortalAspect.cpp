#include "pch.h"
#include "PortalAspect.h"

#include "../ECS/GameplayRuntimeComponents.h"
#include "WorldRuntime.h"

CharacterFeatureFlags PortalAspect::RequiredFeature() const noexcept
{
	return CharacterFeatureFlags::PortalAware;
}

void PortalAspect::RegisterStorages(WorldRuntime& runtime) const
{
	runtime.RegisterStorage<PortalTriggerStateComp>();
}

void PortalAspect::Attach(
	WorldRuntime& runtime,
	Entity entity,
	const CharacterDef& def,
	const AssembleParams& params) const
{
	runtime.DeferredAddComponent<PortalTriggerStateComp>(entity);
}
