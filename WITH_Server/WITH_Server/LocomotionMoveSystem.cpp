#include "pch.h"
#include "LocomotionMoveSystem.h"
#include "Framework.h"
#include "Math.h"

void LocomotionMoveSystem::Execute(const float dT)
{
	auto& velocities = ecs.GetStorage<Velocity>();
	auto& actionStates = ecs.GetStorage<ActionState>();
	auto& locos = ecs.GetStorage<LocomotionState>();
	auto& moveDeltas = ecs.GetStorage<LocomotionMoveDelta>();
	auto& animPhases = ecs.GetStorage<LocomotionAnimPhase>();

	for (const auto& [entity, loco] : locos)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		auto* vel = velocities.GetComponent(entity);
		auto* actionState = actionStates.GetComponent(entity);
		auto* moveDelta = moveDeltas.GetComponent(entity);
		auto* animPhase = animPhases.GetComponent(entity);

		if(!vel || !moveDelta || !actionState || !animPhase) continue;
		if (actionState->type != ActionType::None) continue;
		
		ApplyNormalMovement(moveDelta, animPhase, 
			loco, *vel, dT);
	}
}

void LocomotionMoveSystem::ApplyNormalMovement(
	LocomotionMoveDelta* moveDelta, LocomotionAnimPhase* animPhase, 
	const LocomotionState& loco, const Velocity& vel, const float dT)
{
	moveDelta->hasMove = false;
	moveDelta->hasYaw = false;

	if (!loco.isMoving)
	{
		animPhase->wasMoving = false;
		return;
	}

	else
	{
		if (!animPhase->wasMoving)
		{
			animPhase->phase = 0.0f;
			animPhase->wasMoving = true;
		}

		// TEMP : 걷기 뛰기에 따라 속도 조정
		const float speed = 2.0f;

		XMVECTOR raw = XMLoadFloat3(&vel.dir);
		XMVECTOR dir;
		if (!TransformHelper::SafeNormalize3(raw, dir)) return;

		XMFLOAT3 moveDT;
		XMStoreFloat3(&moveDT, XMVectorScale(dir, speed * dT));

		moveDelta->hasMove = true;
		moveDelta->deltaPos = moveDT;

		const float movedSq = moveDT.x * moveDT.x + 
			moveDT.y * moveDT.y + moveDT.z * moveDT.z;
		if (movedSq > 1e-12f)
		{
			const float yaw = atan2f(-moveDT.x, -moveDT.z);
			moveDelta->hasYaw = true;
			moveDelta->yaw = yaw;
		}

		const float moved = sqrtf(movedSq);
		const float strideLen = 160.0f / 100.0f;

		if (moved > 1e-6f)
		{
			animPhase->phase += moved / strideLen;
			animPhase->phase -= floorf(animPhase->phase);
		}
	}
}