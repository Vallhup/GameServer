#include "pch.h"
#include "ResolveLocomotionStateSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 8> kResolveLocomotionAccesses{
		WriteImmediate(ComponentRes<LocomotionStateComp>()),
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		ReadImmediate(ComponentRes<ActorInputComp>()),
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<AIIntentFrameComp>()),
		ReadImmediate(ComponentRes<SpawnTypeComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
	};

	bool TryGetLookYaw(
		const SystemContext& ctx,
		Entity entity,
		const WorldTransformComp& transform,
		float fallbackYaw,
		float& outYaw)
	{
		const AIIntentFrameComp* aiIntent =
			ctx.ecs.GetComponent<AIIntentFrameComp>(entity);
		if (aiIntent == nullptr ||
			!aiIntent->hasLook ||
			aiIntent->target.IsNull())
		{
			return false;
		}

		const WorldTransformComp* targetTransform =
			ctx.ecs.GetComponent<WorldTransformComp>(aiIntent->target);
		if (targetTransform == nullptr)
		{
			return false;
		}

		float lookDirX = targetTransform->position.x - transform.position.x;
		float lookDirZ = targetTransform->position.z - transform.position.z;
		NormalizeXZ(lookDirX, lookDirZ);
		if (LengthXZ(lookDirX, lookDirZ) <= kOverlapEpsilon)
		{
			return false;
		}

		outYaw = DirToYaw(lookDirX, lookDirZ, fallbackYaw);
		return true;
	}

	LocomotionMode SelectFacingRelativeWalkMode(
		float moveDirX,
		float moveDirZ,
		float facingYawRad,
		bool wantsRun)
	{
		if (wantsRun)
		{
			return LocomotionMode::Run;
		}

		const float sinYaw = std::sin(facingYawRad);
		const float cosYaw = std::cos(facingYawRad);
		const float forwardX = -sinYaw;
		const float forwardZ = -cosYaw;
		const float rightX = -cosYaw;
		const float rightZ = sinYaw;

		const float forwardDot = moveDirX * forwardX + moveDirZ * forwardZ;
		const float rightDot = moveDirX * rightX + moveDirZ * rightZ;

		if (forwardDot < -0.45f)
		{
			return LocomotionMode::WalkBack;
		}

		if (std::abs(rightDot) > 0.55f &&
			std::abs(rightDot) > std::abs(forwardDot))
		{
			return (rightDot >= 0.0f)
				? LocomotionMode::WalkRight
				: LocomotionMode::WalkLeft;
		}

		return LocomotionMode::Walk;
	}
}

const SystemMeta ResolveLocomotionStateSystem::kMeta =
	SystemMeta{
		SysTag<ResolveLocomotionStateSystem>(),
		"ResolveLocomotionStateSystem",
		kResolveLocomotionAccesses,
		kNoDeps,
		kNoDeps
	};

void ResolveLocomotionStateSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, locomotionState, abilityState, input, transform] :
		ctx.ecs.View<
			LocomotionStateComp,
			AbilityStateComp,
			ActorInputComp,
			WorldTransformComp>())
	{
		if (HasBlockingPendingState(ctx.ecs, entity) ||
			IsAbilityActive(abilityState))
		{
			locomotionState.mode = LocomotionMode::Idle;
			locomotionState.desiredMoveDirX = 0.0f;
			locomotionState.desiredMoveDirZ = 0.0f;
			locomotionState.desiredFacingYawRad = locomotionState.facingYawRad;
			locomotionState.currentSpeed = 0.0f;
			locomotionState.wasLocomotionMoving = false;
			continue;
		}

		float inputX = ClampFloat(input.move.inputX, -1.0f, 1.0f);
		float inputZ = ClampFloat(input.move.inputZ, -1.0f, 1.0f);
		NormalizeXZ(inputX, inputZ);

		const CharacterStatDef* statDef = FindCharacterStats(ctx.ecs, entity);
		const float baseSpeed = (statDef != nullptr) ? statDef->moveSpeed : 2.5f;
		const bool moving = LengthXZ(inputX, inputZ) > kOverlapEpsilon;

		if (!moving)
		{
			float lookYaw = locomotionState.facingYawRad;
			if (TryGetLookYaw(ctx, entity, transform, locomotionState.facingYawRad, lookYaw))
			{
				const float yawDelta =
					WrapYaw(lookYaw - locomotionState.facingYawRad);

				locomotionState.desiredMoveDirX = 0.0f;
				locomotionState.desiredMoveDirZ = 0.0f;
				locomotionState.desiredFacingYawRad = lookYaw;
				locomotionState.mode =
					(std::abs(yawDelta) > 0.18f)
					? ((yawDelta > 0.0f)
						? LocomotionMode::TurnRight
						: LocomotionMode::TurnLeft)
					: LocomotionMode::Idle;
				locomotionState.currentSpeed = 0.0f;
				locomotionState.wasLocomotionMoving = false;
				continue;
			}

			locomotionState.desiredMoveDirX = 0.0f;
			locomotionState.desiredMoveDirZ = 0.0f;
			locomotionState.desiredFacingYawRad = locomotionState.facingYawRad;
			locomotionState.mode = LocomotionMode::Idle;
			locomotionState.currentSpeed = 0.0f;
			locomotionState.wasLocomotionMoving = false;
			continue;
		}

		const float yaw = input.move.cameraYawRad;
		const float sinYaw = std::sin(yaw);
		const float cosYaw = std::cos(yaw);

		const float forwardX = sinYaw;
		const float forwardZ = cosYaw;
		const float rightX = cosYaw;
		const float rightZ = -sinYaw;

		float moveDirX = rightX * inputX + forwardX * inputZ;
		float moveDirZ = rightZ * inputX + forwardZ * inputZ;
		NormalizeXZ(moveDirX, moveDirZ);

		float lookYaw = locomotionState.facingYawRad;
		const AIIntentFrameComp* aiIntent =
			ctx.ecs.GetComponent<AIIntentFrameComp>(entity);
		const bool useFacingRelativeLocomotion =
			aiIntent != nullptr && aiIntent->lockFacingToLookTarget;
		const bool hasLookYaw =
			useFacingRelativeLocomotion &&
			TryGetLookYaw(ctx, entity, transform, locomotionState.facingYawRad, lookYaw);

		locomotionState.desiredMoveDirX = moveDirX;
		locomotionState.desiredMoveDirZ = moveDirZ;
		locomotionState.desiredFacingYawRad =
			hasLookYaw
			? lookYaw
			: DirToYaw(moveDirX, moveDirZ, locomotionState.facingYawRad);

		locomotionState.facingYawRad = locomotionState.desiredFacingYawRad;

		locomotionState.mode = hasLookYaw
			? SelectFacingRelativeWalkMode(
				moveDirX,
				moveDirZ,
				locomotionState.facingYawRad,
				input.move.wantsRun)
			: (input.move.wantsRun
				? LocomotionMode::Run
				: LocomotionMode::Walk);
		locomotionState.currentSpeed = baseSpeed *
			(input.move.wantsRun ? 1.0f : kWalkSpeedScale);

		if (!locomotionState.wasLocomotionMoving)
		{
			locomotionState.locomotionAnimPhase01 = 0.0f;
		}
		locomotionState.wasLocomotionMoving = true;
	}
}

const SystemMeta& ResolveLocomotionStateSystem::Meta() const
{
	return kMeta;
}
