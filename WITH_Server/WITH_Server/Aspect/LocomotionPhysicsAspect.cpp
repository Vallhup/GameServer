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
	runtime.RegisterStorage<AbilityMoveDeltaComp>();
	runtime.RegisterStorage<AbilityMoveRuntimeComp>();
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
	(void)params;
	runtime.DeferredAddComponent<LocomotionMoveDeltaComp>(entity);
	runtime.DeferredAddComponent<AbilityMoveDeltaComp>(entity);
	runtime.DeferredAddComponent<AbilityMoveRuntimeComp>(entity);
	runtime.DeferredAddComponent<PreCollisionTransformComp>(entity);

	BodyCollisionShapeComp shape{};
	shape.bodyRadiusXZ = def.bodyCollision.footprintRadiusXZ;
	shape.bodyHeight = def.bodyCollision.bodyHeight;
	shape.blocksBodyOverlap = def.bodyCollision.blocksBodyOverlap;
	shape.useNavMeshConstraint = def.bodyCollision.useNavMeshConstraint;
	shape.pushability = def.bodyCollision.pushability;
	shape.overlapYieldWeight = def.bodyCollision.overlapYieldWeight;
	shape.maxOverlapCorrectionPerFrameXZ =
		def.bodyCollision.maxOverlapCorrectionPerFrameXZ;
	runtime.DeferredUpsertComponent<BodyCollisionShapeComp>(entity, shape);

	runtime.DeferredAddComponent<NavMeshAgentStateComp>(entity);
	runtime.DeferredAddComponent<BodyCollisionResolveComp>(entity);
}
