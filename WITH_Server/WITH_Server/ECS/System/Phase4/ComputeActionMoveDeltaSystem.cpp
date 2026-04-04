#include "pch.h"
#include "ComputeActionMoveDeltaSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta ComputeActionMoveDeltaSystem::kMeta =
	MakeSystemMeta<ComputeActionMoveDeltaSystem>(
		"ComputeActionMoveDeltaSystem");

void ComputeActionMoveDeltaSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, actionState, moveDelta, moveRuntime, advance] :
		ctx.ecs.View<
			ActionStateComp,
			ActionMoveDeltaComp,
			ActionMoveRuntimeComp,
			ActionTimelineAdvanceComp>())
	{
		(void)entity;
		moveDelta = {};

		if (!IsActionActive(actionState) ||
			advance.actionId != actionState.actionId ||
			advance.actionInstanceId != actionState.actionInstanceId)
		{
			moveRuntime = {};
			continue;
		}

		const ActionDef* actionDef = FindActionDef(actionState.actionId);
		if (actionDef == nullptr)
		{
			continue;
		}

		if (moveRuntime.boundActionInstanceId !=
			actionState.actionInstanceId)
		{
			moveRuntime = {};
			moveRuntime.boundActionInstanceId = actionState.actionInstanceId;
		}

		for (const ActionMovementSegmentDef& segment : actionDef->moveSegments)
		{
			const float startSec = segment.startNormalized * actionDef->duration;
			const float endSec = segment.endNormalized * actionDef->duration;
			if (endSec <= startSec)
			{
				continue;
			}

			const float overlapStart = std::max(startSec, advance.prevElapsedSec);
			const float overlapEnd = std::min(endSec, advance.currElapsedSec);
			if (overlapEnd <= overlapStart)
			{
				continue;
			}

			float dirX = actionState.directionX;
			float dirZ = actionState.directionZ;
			NormalizeXZ(dirX, dirZ);
			if (LengthXZ(dirX, dirZ) <= kOverlapEpsilon)
			{
				dirX = std::sin(moveRuntime.lockedYawRad);
				dirZ = std::cos(moveRuntime.lockedYawRad);
			}

			if (!moveRuntime.hasLockedDirection ||
				segment.dirSampleTiming == DirectionSampleTiming::Continuous)
			{
				moveRuntime.lockedDirX = dirX;
				moveRuntime.lockedDirZ = dirZ;
				moveRuntime.lockedYawRad = DirToYaw(dirX, dirZ, 0.0f);
				moveRuntime.hasLockedDirection = true;
			}

			if (segment.moveDistance.has_value())
			{
				const float alpha =
					(overlapEnd - overlapStart) / (endSec - startSec);
				const float distance = segment.moveDistance.value() * alpha;
				moveDelta.deltaPosition.x += moveRuntime.lockedDirX * distance;
				moveDelta.deltaPosition.z += moveRuntime.lockedDirZ * distance;
				moveDelta.hasDelta = true;
			}
		}
	}
}

const SystemMeta& ComputeActionMoveDeltaSystem::Meta() const
{
	return kMeta;
}
