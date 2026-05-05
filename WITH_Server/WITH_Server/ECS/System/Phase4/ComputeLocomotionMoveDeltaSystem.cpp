#include "pch.h"
#include "ComputeLocomotionMoveDeltaSystem.h"

#include "../GameplaySystemUtil.h"
#include "../../../TransformHelper.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 5> kComputeLocomotionMoveDeltaAccesses{
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		WriteImmediate(ComponentRes<LocomotionStateComp>()),
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		ReadImmediate(ComponentRes<AICommandFrameComp>()),
		WriteImmediate(ComponentRes<LocomotionMoveDeltaComp>()),
	};

	bool IsLookOnlyLocomotionMode(LocomotionMode mode) noexcept
	{
		return
			mode == LocomotionMode::Idle ||
			mode == LocomotionMode::TurnLeft ||
			mode == LocomotionMode::TurnRight;
	}
}

const SystemMeta ComputeLocomotionMoveDeltaSystem::kMeta =
	SystemMeta{
		SysTag<ComputeLocomotionMoveDeltaSystem>(),
		"ComputeLocomotionMoveDeltaSystem",
		kComputeLocomotionMoveDeltaAccesses,
		kNoDeps,
		kNoDeps
	};

void ComputeLocomotionMoveDeltaSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, transform, locomotionState, abilityState, moveDelta] :
		ctx.ecs.View<
			WorldTransformComp,
			LocomotionStateComp,
			AbilityStateComp,
			LocomotionMoveDeltaComp>())
	{
		moveDelta = {};

		if (IsAbilityActive(abilityState))
		{
			continue;
		}

		if (IsLookOnlyLocomotionMode(locomotionState.mode))
		{
			const AICommandFrameComp* aiCommand =
				ctx.ecs.GetComponent<AICommandFrameComp>(entity);
			if (aiCommand == nullptr ||
				!aiCommand->hasLook ||
				aiCommand->target.IsNull())
			{
				continue;
			}

			const WorldTransformComp* targetTransform =
				ctx.ecs.GetComponent<WorldTransformComp>(aiCommand->target);
			if (targetTransform == nullptr)
			{
				continue;
			}

			float lookDirX = targetTransform->position.x - transform.position.x;
			float lookDirZ = targetTransform->position.z - transform.position.z;
			NormalizeXZ(lookDirX, lookDirZ);

			if (LengthXZ(lookDirX, lookDirZ) <= kOverlapEpsilon)
			{
				continue;
			}

			const float currYaw =
				TransformHelper::QuaternionToYaw(transform.rotation);
			const float targetYaw =
				DirToYaw(lookDirX, lookDirZ, currYaw);
			const float nextYaw =
				ClampYawStep(
					currYaw,
					targetYaw,
					kYawTurnSpeedRad * static_cast<float>(ctx.dtSec));

			locomotionState.desiredFacingYawRad = nextYaw;
			locomotionState.facingYawRad = nextYaw;

			moveDelta.deltaYawRad =
				TransformHelper::AngleDelta(currYaw, nextYaw);
			moveDelta.hasDelta =
				std::abs(moveDelta.deltaYawRad) > kOverlapEpsilon;
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
