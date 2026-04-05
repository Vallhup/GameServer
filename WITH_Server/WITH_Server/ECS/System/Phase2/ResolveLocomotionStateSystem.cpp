#include "pch.h"
#include "ResolveLocomotionStateSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta ResolveLocomotionStateSystem::kMeta =
	MakeSystemMeta<ResolveLocomotionStateSystem>("ResolveLocomotionStateSystem");

void ResolveLocomotionStateSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, locomotionState, actionState, input] :
		ctx.ecs.View<LocomotionStateComp, ActionStateComp, ActorInputComp>())
	{
		if (HasBlockingPendingState(ctx.ecs, entity) || IsActionActive(actionState))
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

		locomotionState.desiredMoveDirX = moveDirX;
		locomotionState.desiredMoveDirZ = moveDirZ;
		locomotionState.desiredFacingYawRad =
			DirToYaw(moveDirX, moveDirZ, locomotionState.facingYawRad);

		locomotionState.facingYawRad = locomotionState.desiredFacingYawRad;

		locomotionState.mode = input.move.wantsRun
			? LocomotionMode::Run
			: LocomotionMode::Walk;
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
