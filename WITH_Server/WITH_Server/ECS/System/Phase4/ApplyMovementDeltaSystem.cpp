#include "pch.h"
#include "ApplyMovementDeltaSystem.h"

#include "../GameplaySystemUtil.h"
#include "../../../TransformHelper.h"

using namespace GameplaySystemUtil;

const StaticSystemMetaStorage<6> ApplyMovementDeltaSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<ApplyMovementDeltaSystem>(),
		"ApplyMovementDeltaSystem",
		std::array<AccessSpec, 6>
	{
		WriteImmediate(ComponentRes<WorldTransformComp>()),
		WriteImmediate(ComponentRes<PreCollisionTransformComp>()),
		WriteImmediate(ComponentRes<LocomotionMoveDeltaComp>()),
		WriteImmediate(ComponentRes<AbilityMoveDeltaComp>()),
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		WriteImmediate(ComponentRes<DirtyFlagsComp>()),
	});

void ApplyMovementDeltaSystem::Execute(SystemContext& ctx)
{
	for (auto [
		entity,
		transform,
		preCollision,
		locomotionDelta,
		abilityDelta,
		abilityState] :
		ctx.ecs.View<
			WorldTransformComp,
			PreCollisionTransformComp,
			LocomotionMoveDeltaComp,
			AbilityMoveDeltaComp,
			AbilityStateComp>())
	{
		preCollision.prevPosition = transform.position;
		preCollision.prevRotation = transform.rotation;
		preCollision.candidatePosition = transform.position;
		preCollision.candidateRotation = transform.rotation;
		preCollision.movedThisFrame = false;
		preCollision.rotatedThisFrame = false;
		preCollision.preserveAbilityVerticalAboveNavMesh = false;

		if (abilityDelta.hasDelta)
		{
			preCollision.preserveAbilityVerticalAboveNavMesh =
				std::abs(abilityDelta.deltaPosition.y) > kOverlapEpsilon;
			transform.position.x += abilityDelta.deltaPosition.x;
			transform.position.y += abilityDelta.deltaPosition.y;
			transform.position.z += abilityDelta.deltaPosition.z;
			TransformHelper::ApplyYawRotation(transform,abilityDelta.deltaYawRad);
		}
		else if (!IsAbilityActive(abilityState) && locomotionDelta.hasDelta)
		{
			transform.position.x += locomotionDelta.deltaPosition.x;
			transform.position.y += locomotionDelta.deltaPosition.y;
			transform.position.z += locomotionDelta.deltaPosition.z;
			TransformHelper::ApplyYawRotation(transform,locomotionDelta.deltaYawRad);
		}

		preCollision.candidatePosition = transform.position;
		preCollision.candidateRotation = transform.rotation;
		preCollision.movedThisFrame =
			LengthXZ(
				transform.position.x - preCollision.prevPosition.x,
				transform.position.z - preCollision.prevPosition.z) >
			kOverlapEpsilon;

		const float prevYaw = TransformHelper::QuaternionToYaw(preCollision.prevRotation);
		const float currYaw = TransformHelper::QuaternionToYaw(transform.rotation);

		preCollision.rotatedThisFrame =
			std::abs(TransformHelper::AngleDelta(prevYaw, currYaw)) > kOverlapEpsilon;

		abilityDelta = {};
		locomotionDelta = {};

		if ((preCollision.movedThisFrame ||
			preCollision.rotatedThisFrame ||
			preCollision.preserveAbilityVerticalAboveNavMesh) &&
			ctx.ecs.HasComponent<DirtyFlagsComp>(entity))
		{
			ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity)
				->MarkDirty(WorldDirtyType::Transform);
		}
	}
}
