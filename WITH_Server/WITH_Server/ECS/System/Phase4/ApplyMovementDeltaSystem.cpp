#include "pch.h"
#include "ApplyMovementDeltaSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta ApplyMovementDeltaSystem::kMeta =
	MakeSystemMeta<ApplyMovementDeltaSystem>("ApplyMovementDeltaSystem");

void ApplyMovementDeltaSystem::Execute(SystemContext& ctx)
{
	for (auto [
		entity,
		transform,
		preCollision,
		locomotionDelta,
		actionDelta,
		actionState] :
		ctx.ecs.View<
			WorldTransformComp,
			PreCollisionTransformComp,
			LocomotionMoveDeltaComp,
			ActionMoveDeltaComp,
			ActionStateComp>())
	{
		preCollision.prevPosition = transform.position;
		preCollision.prevYawRad = transform.yawRad;
		preCollision.candidatePosition = transform.position;
		preCollision.candidateYawRad = transform.yawRad;
		preCollision.movedThisFrame = false;
		preCollision.rotatedThisFrame = false;

		if (actionDelta.hasDelta)
		{
			transform.position.x += actionDelta.deltaPosition.x;
			transform.position.y += actionDelta.deltaPosition.y;
			transform.position.z += actionDelta.deltaPosition.z;
			transform.yawRad = WrapYaw(
				transform.yawRad + actionDelta.deltaYawRad);
		}
		else if (!IsActionActive(actionState) && locomotionDelta.hasDelta)
		{
			transform.position.x += locomotionDelta.deltaPosition.x;
			transform.position.y += locomotionDelta.deltaPosition.y;
			transform.position.z += locomotionDelta.deltaPosition.z;
			transform.yawRad = WrapYaw(
				transform.yawRad + locomotionDelta.deltaYawRad);
		}

		preCollision.candidatePosition = transform.position;
		preCollision.candidateYawRad = transform.yawRad;
		preCollision.movedThisFrame =
			LengthXZ(
				transform.position.x - preCollision.prevPosition.x,
				transform.position.z - preCollision.prevPosition.z) >
			kOverlapEpsilon;
		preCollision.rotatedThisFrame =
			std::abs(WrapYaw(transform.yawRad - preCollision.prevYawRad)) >
			kOverlapEpsilon;

		actionDelta = {};
		locomotionDelta = {};

		if ((preCollision.movedThisFrame || preCollision.rotatedThisFrame) &&
			ctx.ecs.HasComponent<DirtyFlagsComp>(entity))
		{
			MutableComponent<DirtyFlagsComp>(ctx.ecs, entity)
				->MarkDirty(WorldDirtyType::Transform);
		}
	}
}

const SystemMeta& ApplyMovementDeltaSystem::Meta() const
{
	return kMeta;
}
