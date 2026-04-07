#include "pch.h"
#include "ComputeLocomotionMoveDeltaSystem.h"

#include "../GameplaySystemUtil.h"
#include "../../../TransformHelper.h"

using namespace GameplaySystemUtil;

const SystemMeta ComputeLocomotionMoveDeltaSystem::kMeta =
	MakeSystemMeta<ComputeLocomotionMoveDeltaSystem>(
		"ComputeLocomotionMoveDeltaSystem");

void ComputeLocomotionMoveDeltaSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, transform, locomotionState, actionState, moveDelta] :
		ctx.ecs.View<
			WorldTransformComp,
			LocomotionStateComp,
			ActionStateComp,
			LocomotionMoveDeltaComp>())
	{
		(void)entity;
		moveDelta = {};

		if (IsActionActive(actionState) ||
			locomotionState.mode == LocomotionMode::Idle)
		{
			continue;
		}

		float dirX = locomotionState.desiredMoveDirX;
		float dirZ = locomotionState.desiredMoveDirZ;
		NormalizeXZ(dirX, dirZ);

		if (LengthXZ(dirX, dirZ) <= kOverlapEpsilon)
		{
			continue;
		}

		const float moveDist =
			locomotionState.currentSpeed * static_cast<float>(ctx.dtSec);

		moveDelta.deltaPosition.x = dirX * moveDist;
		moveDelta.deltaPosition.y = 0.0f;
		moveDelta.deltaPosition.z = dirZ * moveDist;
	
		const float currYaw = TransformHelper::QuaternionToYaw(transform.rotation);

		moveDelta.deltaYawRad =
			TransformHelper::AngleDelta(currYaw, locomotionState.facingYawRad);

		moveDelta.hasDelta = true;
	}
}

const SystemMeta& ComputeLocomotionMoveDeltaSystem::Meta() const
{
	return kMeta;
}
