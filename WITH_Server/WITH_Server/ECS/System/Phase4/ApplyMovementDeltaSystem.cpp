#include "pch.h"
#include "ApplyMovementDeltaSystem.h"

#include "../GameplaySystemUtil.h"
#include "../../../TransformHelper.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 6> kApplyMovementDeltaAccesses{
		WriteImmediate(ComponentRes<WorldTransformComp>()),
		WriteImmediate(ComponentRes<PreCollisionTransformComp>()),
		WriteImmediate(ComponentRes<LocomotionMoveDeltaComp>()),
		WriteImmediate(ComponentRes<ActionMoveDeltaComp>()),
		ReadImmediate(ComponentRes<ActionStateComp>()),
		WriteImmediate(ComponentRes<DirtyFlagsComp>()),
	};
}

const SystemMeta ApplyMovementDeltaSystem::kMeta =
	SystemMeta{
		SysTag<ApplyMovementDeltaSystem>(),
		"ApplyMovementDeltaSystem",
		kApplyMovementDeltaAccesses,
		kNoDeps,
		kNoDeps
	};

void ApplyMovementDeltaSystem::Execute(SystemContext& ctx)
{
	auto ApplyDeltaYaw =
		[](WorldTransformComp& tr, float deltaYaw)
		{
			if (std::abs(deltaYaw) <= kOverlapEpsilon)
				return;

			XMVECTOR curRot = XMLoadFloat4(&tr.rotation);
			XMVECTOR deltaRot = XMQuaternionRotationAxis(
				XMVectorSet(0.f, 1.f, 0.f, 0.f),
				deltaYaw);
			XMVECTOR nextRot = XMQuaternionMultiply(deltaRot, curRot);
			nextRot = XMQuaternionNormalize(nextRot);
			XMStoreFloat4(&tr.rotation, nextRot);
		};

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
		preCollision.prevRotation = transform.rotation;
		preCollision.candidatePosition = transform.position;
		preCollision.candidateRotation = transform.rotation;
		preCollision.movedThisFrame = false;
		preCollision.rotatedThisFrame = false;

		if (actionDelta.hasDelta)
		{
			transform.position.x += actionDelta.deltaPosition.x;
			transform.position.y += actionDelta.deltaPosition.y;
			transform.position.z += actionDelta.deltaPosition.z;
			ApplyDeltaYaw(transform, actionDelta.deltaYawRad);
		}
		else if (!IsActionActive(actionState) && locomotionDelta.hasDelta)
		{
			transform.position.x += locomotionDelta.deltaPosition.x;
			transform.position.y += locomotionDelta.deltaPosition.y;
			transform.position.z += locomotionDelta.deltaPosition.z;
			ApplyDeltaYaw(transform, locomotionDelta.deltaYawRad);
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

		actionDelta = {};
		locomotionDelta = {};

		if ((preCollision.movedThisFrame || preCollision.rotatedThisFrame) &&
			ctx.ecs.HasComponent<DirtyFlagsComp>(entity))
		{
			ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity)
				->MarkDirty(WorldDirtyType::Transform);
		}
	}
}

const SystemMeta& ApplyMovementDeltaSystem::Meta() const
{
	return kMeta;
}
