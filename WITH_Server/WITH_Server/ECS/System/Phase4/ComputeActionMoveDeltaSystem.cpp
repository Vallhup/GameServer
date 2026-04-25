#include "pch.h"
#include "ComputeActionMoveDeltaSystem.h"

#include "../GameplaySystemUtil.h"
#include "../../../TransformHelper.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 5> kComputeActionMoveDeltaAccesses{
		ReadSnapshot(ComponentRes<ActionStateComp>()),
		ReadSnapshot(ComponentRes<WorldTransformComp>()),
		WriteImmediate(ComponentRes<ActionMoveDeltaComp>()),
		WriteImmediate(ComponentRes<ActionMoveRuntimeComp>()),
		ReadSnapshot(ComponentRes<ActionTimelineAdvanceComp>()),
	};

	void DirectionFromTransform(
		const WorldTransformComp& transform,
		float& outDirX, 
		float& outDirZ)
	{
		XMVECTOR fwd = TransformHelper::Forward(transform);
		outDirX = XMVectorGetX(fwd);
		outDirZ = XMVectorGetZ(fwd);
		NormalizeXZ(outDirX, outDirZ);
	}

	bool HasDirection(float dirX, float dirZ)
	{
		return LengthXZ(dirX, dirZ) > kOverlapEpsilon;
	}

	bool EnteredSegmentThisFrame(
		const ActionTimelineAdvanceComp& advance,
		float segmentStartSec)
	{
		return
			segmentStartSec + kOverlapEpsilon >= advance.prevElapsedSec &&
			segmentStartSec < advance.currElapsedSec;
	}

	void ResolvePolicyDirection(
		const ActionMovementSegmentDef& segment,
		const ActionStateComp& actionState,
		const WorldTransformComp& transform,
		const ActionMoveRuntimeComp& moveRuntime,
		float& outDirX,
		float& outDirZ)
	{
		outDirX = 0.0f;
		outDirZ = 0.0f;

		switch (segment.dirPolicy) {
		case DirectionPolicy::ActionStartInput:
		case DirectionPolicy::CurrentInput:
		case DirectionPolicy::TargetDirection:
			outDirX = actionState.directionX;
			outDirZ = actionState.directionZ;
			NormalizeXZ(outDirX, outDirZ);
			break;
		case DirectionPolicy::FacingDirection:
			DirectionFromTransform(transform, outDirX, outDirZ);
			break;
		case DirectionPolicy::LockedDirection:
			if (moveRuntime.hasLockedDirection)
			{
				outDirX = moveRuntime.lockedDirX;
				outDirZ = moveRuntime.lockedDirZ;
				NormalizeXZ(outDirX, outDirZ);
			}
			break;
		default:
			break;
		}

		if (!HasDirection(outDirX, outDirZ) && moveRuntime.hasLockedDirection)
		{
			outDirX = moveRuntime.lockedDirX;
			outDirZ = moveRuntime.lockedDirZ;
			NormalizeXZ(outDirX, outDirZ);
		}

		if (!HasDirection(outDirX, outDirZ))
		{
			DirectionFromTransform(transform, outDirX, outDirZ);
		}
	}

	void SampleSegmentDirection(
		const ActionMovementSegmentDef& segment,
		const ActionTimelineAdvanceComp& advance,
		const ActionStateComp& actionState,
		const WorldTransformComp& transform,
		ActionMoveRuntimeComp& moveRuntime,
		float segmentStartSec)
	{
		const bool shouldSample =
			!moveRuntime.hasLockedDirection ||
			segment.dirSampleTiming == DirectionSampleTiming::Continuous ||
			(segment.dirPolicy != DirectionPolicy::LockedDirection &&
				EnteredSegmentThisFrame(advance, segmentStartSec));
		if (!shouldSample)
		{
			return;
		}

		float dirX = 0.0f;
		float dirZ = 0.0f;
		ResolvePolicyDirection(
			segment,
			actionState,
			transform,
			moveRuntime,
			dirX,
			dirZ);

		if (!HasDirection(dirX, dirZ))
		{
			return;
		}

		const float currYaw =
			TransformHelper::QuaternionToYaw(transform.rotation);

		moveRuntime.lockedDirX = dirX;
		moveRuntime.lockedDirZ = dirZ;
		moveRuntime.lockedYawRad = DirToYaw(dirX, dirZ, currYaw);
		moveRuntime.hasLockedDirection = true;
	}

	void ApplyRotationTowardYaw(
		ActionMoveDeltaComp& moveDelta,
		const WorldTransformComp& transform,
		float targetYaw,
		float maxStep)
	{
		const float currYaw =
			TransformHelper::QuaternionToYaw(transform.rotation);
		const float nextYaw = (maxStep > kOverlapEpsilon)
			? ClampYawStep(currYaw, targetYaw, maxStep)
			: targetYaw;

		moveDelta.deltaYawRad =
			WrapYaw(moveDelta.deltaYawRad + WrapYaw(nextYaw - currYaw));
		moveDelta.hasDelta = true;
	}
}

const SystemMeta ComputeActionMoveDeltaSystem::kMeta =
	SystemMeta{
		SysTag<ComputeActionMoveDeltaSystem>(),
		"ComputeActionMoveDeltaSystem",
		kComputeActionMoveDeltaAccesses,
		kNoDeps,
		kNoDeps
	};

void ComputeActionMoveDeltaSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, actionState, transform, moveDelta, moveRuntime, advance] :
		ctx.ecs.View<
			ActionStateComp,
			WorldTransformComp,
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

			SampleSegmentDirection(
				segment,
				advance,
				actionState,
				transform,
				moveRuntime,
				startSec);

			const float alpha =
				(overlapEnd - overlapStart) / (endSec - startSec);

			XMFLOAT3 segmentDelta{ 0.0f, 0.0f, 0.0f };
			if (segment.horizontalMoveMode != HorizontalMovementMode::None &&
				segment.moveDistance.has_value() &&
				moveRuntime.hasLockedDirection)
			{
				const float distance = segment.moveDistance.value() * alpha;
				segmentDelta.x = moveRuntime.lockedDirX * distance;
				segmentDelta.z = moveRuntime.lockedDirZ * distance;
				moveDelta.deltaPosition.x += segmentDelta.x;
				moveDelta.deltaPosition.z += segmentDelta.z;
				moveDelta.hasDelta = true;
			}

			if (segment.verticalMoveMode == VerticalMovementMode::FixedOffset &&
				segment.verticalAmount.has_value())
			{
				segmentDelta.y = segment.verticalAmount.value() * alpha;
				moveDelta.deltaPosition.y += segmentDelta.y;
				moveDelta.hasDelta = true;
			}

			if (segment.rotationMode == RotationMode::FaceMoveDirection &&
				HasDirection(segmentDelta.x, segmentDelta.z))
			{
				const float currYaw =
					TransformHelper::QuaternionToYaw(transform.rotation);

				const float targetYaw =
					DirToYaw(segmentDelta.x, segmentDelta.z, currYaw);

				const float maxStep = 
					segment.rotationRate.has_value()
					? segment.rotationRate.value() * (overlapEnd - overlapStart)
					: 0.0f;

				ApplyRotationTowardYaw(moveDelta, transform, targetYaw, maxStep);
			}
			else if (segment.rotationMode == RotationMode::FaceTarget &&
				moveRuntime.hasLockedDirection)
			{
				const float currYaw =
					TransformHelper::QuaternionToYaw(transform.rotation);

				const float targetYaw =
					DirToYaw( moveRuntime.lockedDirX, moveRuntime.lockedDirZ, currYaw);
				const float maxStep = 
					segment.rotationRate.has_value()
					? segment.rotationRate.value() * (overlapEnd - overlapStart)
					: 0.0f;

				ApplyRotationTowardYaw(moveDelta, transform, targetYaw, maxStep);
			}
		}
	}
}

const SystemMeta& ComputeActionMoveDeltaSystem::Meta() const
{
	return kMeta;
}
