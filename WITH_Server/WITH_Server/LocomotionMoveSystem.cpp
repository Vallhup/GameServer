#include "pch.h"
#include "LocomotionMoveSystem.h"
#include "Math.h"
#include "Tags.h"

void LocomotionMoveSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	const auto& velocities = ecs.GetStorage<Velocity>();
	const auto& actionStates = ecs.GetStorage<ActionState>();
	const auto& locos = ecs.GetStorage<LocomotionState>();
	const auto& attributes = ecs.GetStorage<Attribute>();

	auto& moveDeltas = ecs.GetStorage<LocomotionMoveDelta>();
	auto& animPhases = ecs.GetStorage<LocomotionAnimPhase>();

	for (const auto& [entity, loco] : locos)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		const auto* vel = velocities.GetComponent(entity);
		const auto* actionState = actionStates.GetComponent(entity);
		const auto* attribute = attributes.GetComponent(entity);

		auto* moveDelta = moveDeltas.GetComponent(entity);
		auto* animPhase = animPhases.GetComponent(entity);

		if(!vel || !moveDelta || !actionState || !animPhase || !attribute) continue;
		if (actionState->action != ActionType::None) continue;
		
		ApplyNormalMovement(moveDelta, animPhase, loco, *vel, *attribute, dT);
	}
}

void LocomotionMoveSystem::ApplyNormalMovement(
	LocomotionMoveDelta* moveDelta, LocomotionAnimPhase* animPhase, 
	const LocomotionState& loco, const Velocity& vel, const Attribute& attribute, const double dT)
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
			animPhase->phase = 0.0;
			animPhase->wasMoving = true;
		}

		// TEMP : 걷기 뛰기에 따라 속도 조정
		float speed = static_cast<float>(attribute.moveSpeed);
		if (loco.isRun) speed *= 2;

		XMVECTOR raw = XMLoadFloat3(&vel.dir);
		XMVECTOR dir;
		if (!TransformHelper::SafeNormalize3(raw, dir)) return;

		XMFLOAT3 moveDT;
		XMStoreFloat3(&moveDT, XMVectorScale(dir, speed * (float)dT));

		moveDelta->hasMove = true;
		moveDelta->deltaPos = moveDT;

		const float movedSq = 
			moveDT.x * moveDT.x + moveDT.y * moveDT.y + moveDT.z * moveDT.z;
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