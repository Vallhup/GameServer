#include "pch.h"
#include "LocomotionPhysicsAspect.h"

#include "../ECS/GameplayRuntimeComponents.h"
#include "WorldRuntime.h"

CharacterFeatureFlags LocomotionPhysicsAspect::RequiredFeature() const noexcept
{
	return CharacterFeatureFlags::Combatant;
}

void LocomotionPhysicsAspect::RegisterStorages(WorldRuntime& runtime) const
{
	runtime.RegisterStorage<LocomotionMoveDeltaComp>();
	runtime.RegisterStorage<ActionMoveDeltaComp>();
	runtime.RegisterStorage<ActionMoveRuntimeComp>();
	runtime.RegisterStorage<PreCollisionTransformComp>();
	runtime.RegisterStorage<BodyCollisionShapeComp>();
	runtime.RegisterStorage<NavMeshAgentStateComp>();
	runtime.RegisterStorage<BodyCollisionResolveComp>();
}

void LocomotionPhysicsAspect::Attach(
	WorldRuntime& runtime,
	Entity entity,
	const CharacterDef& def,
	const AssembleParams& params) const
{
	(void)def;
	(void)params;
	runtime.DeferredAddComponent<LocomotionMoveDeltaComp>(entity);
	runtime.DeferredAddComponent<ActionMoveDeltaComp>(entity);
	runtime.DeferredAddComponent<ActionMoveRuntimeComp>(entity);
	runtime.DeferredAddComponent<PreCollisionTransformComp>(entity);
	runtime.DeferredAddComponent<BodyCollisionShapeComp>(entity);
	runtime.DeferredAddComponent<NavMeshAgentStateComp>(entity);
	runtime.DeferredAddComponent<BodyCollisionResolveComp>(entity);
}
