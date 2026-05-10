#include "pch.h"
#include "ComputeAbilityMoveDeltaSystem.h"

#include "../GameplaySystemUtil.h"
#include "../../../TransformHelper.h"

#include <cmath>

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 5> kComputeAbilityMoveDeltaAccesses{
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		WriteImmediate(ComponentRes<AbilityMoveDeltaComp>()),
		WriteImmediate(ComponentRes<AbilityMoveRuntimeComp>()),
		ReadImmediate(ComponentRes<AbilityTimelineAdvanceComp>()),
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
		const AbilityTimelineAdvanceComp& advance,
		float segmentStartSec)
	{
		return
			segmentStartSec + kOverlapEpsilon >= advance.prevElapsedSec &&
			segmentStartSec < advance.currElapsedSec;
	}

	void ResolvePolicyDirection(
		const AbilityMovementSegmentDef& segment,
		const AbilityStateComp& abilityState,
		const WorldTransformComp& transform,
		const AbilityMoveRuntimeComp& moveRuntime,
		float& outDirX,
		float& outDirZ)
	{
		outDirX = 0.0f;
		outDirZ = 0.0f;

		switch (segment.directionPolicy) {
		case AbilityDirectionPolicy::ActionStartInput:
		case AbilityDirectionPolicy::CurrentInput:
		case AbilityDirectionPolicy::TargetDirection:
			outDirX = abilityState.directionX;
			outDirZ = abilityState.directionZ;
			NormalizeXZ(outDirX, outDirZ);
			break;
		case AbilityDirectionPolicy::FacingDirection:
			DirectionFromTransform(transform, outDirX, outDirZ);
			break;
		case AbilityDirectionPolicy::LockedDirection:
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
		const AbilityMovementSegmentDef& segment,
		const AbilityTimelineAdvanceComp& advance,
		const AbilityStateComp& abilityState,
		const WorldTransformComp& transform,
		AbilityMoveRuntimeComp& moveRuntime,
		float segmentStartSec)
	{
		const bool shouldSample =
			!moveRuntime.hasLockedDirection ||
			segment.directionSampleTiming == AbilityDirectionSampleTiming::Continuous ||
			(segment.directionPolicy != AbilityDirectionPolicy::LockedDirection &&
				EnteredSegmentThisFrame(advance, segmentStartSec));
		if (!shouldSample)
		{
			return;
		}

		float dirX = 0.0f;
		float dirZ = 0.0f;
		ResolvePolicyDirection(
			segment,
			abilityState,
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
		AbilityMoveDeltaComp& moveDelta,
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

	bool EnsureTargetDashLock(
		SystemContext& ctx,
		const AbilityMovementSegmentDef& segment,
		const AbilityStateComp& abilityState,
		const WorldTransformComp& transform,
		AbilityMoveRuntimeComp& moveRuntime,
		float segmentStartSec)
	{
		const bool needsLock =
			!moveRuntime.hasLockedTargetDash ||
			std::fabs(moveRuntime.targetDashSegmentStartSec - segmentStartSec) >
				kOverlapEpsilon;
		if (!needsLock)
		{
			return true;
		}

		if (abilityState.target.IsNull())
		{
			return false;
		}

		const WorldTransformComp* targetTransform =
			ctx.ecs.GetComponent<WorldTransformComp>(abilityState.target);
		if (targetTransform == nullptr)
		{
			return false;
		}

		moveRuntime.targetDashSegmentStartSec = segmentStartSec;
		moveRuntime.targetDashStartX = transform.position.x;
		moveRuntime.targetDashStartZ = transform.position.z;
		moveRuntime.targetDashTargetX = targetTransform->position.x;
		moveRuntime.targetDashTargetZ = targetTransform->position.z;
		moveRuntime.hasLockedTargetDash = true;

		float dirX =
			moveRuntime.targetDashTargetX - moveRuntime.targetDashStartX;
		float dirZ =
			moveRuntime.targetDashTargetZ - moveRuntime.targetDashStartZ;
		if (HasDirection(dirX, dirZ))
		{
			NormalizeXZ(dirX, dirZ);
			const float currYaw =
				TransformHelper::QuaternionToYaw(transform.rotation);
			moveRuntime.lockedDirX = dirX;
			moveRuntime.lockedDirZ = dirZ;
			moveRuntime.lockedYawRad = DirToYaw(dirX, dirZ, currYaw);
			moveRuntime.hasLockedDirection = true;
		}

		(void)segment;
		return true;
	}

	void ApplyTargetDashDelta(
		AbilityMoveDeltaComp& moveDelta,
		AbilityMoveRuntimeComp& moveRuntime,
		float segmentStartSec,
		float segmentEndSec,
		float overlapStart,
		float overlapEnd,
		XMFLOAT3& outSegmentDelta)
	{
		const float segmentDuration = segmentEndSec - segmentStartSec;
		if (segmentDuration <= kOverlapEpsilon)
		{
			return;
		}

		const float prevAlpha = ClampFloat(
			(overlapStart - segmentStartSec) / segmentDuration,
			0.0f,
			1.0f);
		const float currAlpha = ClampFloat(
			(overlapEnd - segmentStartSec) / segmentDuration,
			0.0f,
			1.0f);

		const float totalX =
			moveRuntime.targetDashTargetX - moveRuntime.targetDashStartX;
		const float totalZ =
			moveRuntime.targetDashTargetZ - moveRuntime.targetDashStartZ;
		outSegmentDelta.x = totalX * (currAlpha - prevAlpha);
		outSegmentDelta.z = totalZ * (currAlpha - prevAlpha);

		if (HasDirection(outSegmentDelta.x, outSegmentDelta.z))
		{
			moveDelta.deltaPosition.x += outSegmentDelta.x;
			moveDelta.deltaPosition.z += outSegmentDelta.z;
			moveDelta.hasDelta = true;
		}
	}
}

const StaticSystemMetaStorage<5> ComputeAbilityMoveDeltaSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<ComputeAbilityMoveDeltaSystem>(),
		"ComputeAbilityMoveDeltaSystem",
		std::array<AccessSpec, 5>
	{
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		WriteImmediate(ComponentRes<AbilityMoveDeltaComp>()),
		WriteImmediate(ComponentRes<AbilityMoveRuntimeComp>()),
		ReadImmediate(ComponentRes<AbilityTimelineAdvanceComp>()),
	});

void ComputeAbilityMoveDeltaSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, abilityState, transform, moveDelta, moveRuntime, advance] :
		ctx.ecs.View<
			AbilityStateComp,
			WorldTransformComp,
			AbilityMoveDeltaComp,
			AbilityMoveRuntimeComp,
			AbilityTimelineAdvanceComp>())
	{
		(void)entity;
		moveDelta = {};

		if (!IsAbilityActive(abilityState) ||
			advance.abilityId != abilityState.abilityId ||
			advance.abilityInstanceId != abilityState.abilityInstanceId)
		{
			moveRuntime = {};
			continue;
		}

		const AbilityDef* abilityDef =
			GameplayContentCatalogSnapshot::Current().Abilities().Find(abilityState.abilityId);
		if (abilityDef == nullptr)
		{
			continue;
		}

		if (moveRuntime.boundAbilityInstanceId !=
			abilityState.abilityInstanceId)
		{
			moveRuntime = {};
			moveRuntime.boundAbilityInstanceId = abilityState.abilityInstanceId;
		}

		for (const AbilityMovementSegmentDef& segment : abilityDef->timeline.movementSegments)
		{
			const float startSec = segment.startNormalized * abilityDef->timeline.durationSec;
			const float endSec = segment.endNormalized * abilityDef->timeline.durationSec;
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
				abilityState,
				transform,
				moveRuntime,
				startSec);

			const float alpha =
				(overlapEnd - overlapStart) / (endSec - startSec);

			XMFLOAT3 segmentDelta{ 0.0f, 0.0f, 0.0f };
			if (segment.movementMode == AbilityMovementMode::DashToTarget)
			{
				if (EnsureTargetDashLock(
					ctx,
					segment,
					abilityState,
					transform,
					moveRuntime,
					startSec))
				{
					ApplyTargetDashDelta(
						moveDelta,
						moveRuntime,
						startSec,
						endSec,
						overlapStart,
						overlapEnd,
						segmentDelta);
				}
			}
			else if (segment.movementMode != AbilityMovementMode::None &&
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

			if (segment.verticalMovementMode == AbilityVerticalMovementMode::FixedOffset &&
				segment.verticalAmount.has_value())
			{
				segmentDelta.y = segment.verticalAmount.value() * alpha;
				moveDelta.deltaPosition.y += segmentDelta.y;
				moveDelta.hasDelta = true;
			}

			if (segment.rotationMode == AbilityRotationMode::FaceMoveDirection &&
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
			else if (segment.rotationMode == AbilityRotationMode::FaceTarget &&
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
