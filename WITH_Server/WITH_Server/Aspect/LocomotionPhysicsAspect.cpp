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
	runtime.DeferredAddComponent<LocomotionMoveDeltaComp>(entity);
	runtime.DeferredAddComponent<AbilityMoveDeltaComp>(entity);
	runtime.DeferredAddComponent<AbilityMoveRuntimeComp>(entity);
	runtime.DeferredAddComponent<PreCollisionTransformComp>(entity);


	CharacterBodyCollisionDef bodyCollision = def.bodyCollision;

	runtime.DeferredUpsertComponent<BodyCollisionShapeComp>(entity, 
		BodyCollisionShapeComp
		{
			.bodyRadiusXZ					= bodyCollision.footprintRadiusXZ,
			.bodyHeight						= bodyCollision.bodyHeight,
			.blocksBodyOverlap				= bodyCollision.blocksBodyOverlap,
			.useNavMeshConstraint			= bodyCollision.useNavMeshConstraint,
			.pushability					= bodyCollision.pushability,
			.overlapYieldWeight				= bodyCollision.overlapYieldWeight,
			.maxOverlapCorrectionPerFrameXZ = bodyCollision.maxOverlapCorrectionPerFrameXZ
		});

	runtime.DeferredAddComponent<NavMeshAgentStateComp>(entity);
	runtime.DeferredAddComponent<BodyCollisionResolveComp>(entity);
}
