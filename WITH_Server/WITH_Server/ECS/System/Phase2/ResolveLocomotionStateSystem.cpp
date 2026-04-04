#include "pch.h"
#include "ResolveLocomotionStateSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta ResolveLocomotionStateSystem::kMeta =
	MakeSystemMeta<ResolveLocomotionStateSystem>("ResolveLocomotionStateSystem");

void ResolveLocomotionStateSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, locomotionState, actionState, input] :
		ctx.ecs.View<LocomotionStateComp, ActionStateComp, PlayerInputComp>())
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

		float moveDirX = input.move.inputX;
		float moveDirZ = input.move.inputZ;
		NormalizeXZ(moveDirX, moveDirZ);

		const CharacterStatDef* statDef = FindCharacterStats(ctx.ecs, entity);
		const float baseSpeed = (statDef != nullptr) ? statDef->moveSpeed : 2.5f;
		const bool moving = LengthXZ(moveDirX, moveDirZ) > kOverlapEpsilon;

		locomotionState.desiredMoveDirX = moveDirX;
		locomotionState.desiredMoveDirZ = moveDirZ;
		locomotionState.desiredFacingYawRad = moving
			? DirToYaw(moveDirX, moveDirZ, locomotionState.facingYawRad)
			: locomotionState.facingYawRad;
		locomotionState.facingYawRad = ClampYawStep(
			locomotionState.facingYawRad,
			locomotionState.desiredFacingYawRad,
			kYawTurnSpeedRad * static_cast<float>(ctx.dtSec));

		if (!moving)
		{
			locomotionState.mode = LocomotionMode::Idle;
			locomotionState.currentSpeed = 0.0f;
			locomotionState.wasLocomotionMoving = false;
			continue;
		}

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
