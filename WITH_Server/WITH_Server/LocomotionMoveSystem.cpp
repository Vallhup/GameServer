#include "pch.h"
#include "LocomotionMoveSystem.h"
#include "Framework.h"

void LocomotionMoveSystem::Execute(const float dT)
{
	auto& transforms = ecs.GetStorage<Transform>();
	auto& velocities = ecs.GetStorage<Velocity>();
	auto& locos = ecs.GetStorage<LocomotionState>();
	auto& actionStates = ecs.GetStorage<ActionState>();

	for (const auto& [entity, transform] : transforms)
	{
		auto* vel = velocities.GetComponent(entity);
		auto* loco = locos.GetComponent(entity);
		auto* actionState = actionStates.GetComponent(entity);

		if(!vel || !loco || !actionState) continue;
		if (actionState->type != ActionType::None) continue;

		ApplyNormalMovement(entity, *loco, transform, *vel, dT);
	}
}

void LocomotionMoveSystem::ApplyNormalMovement(Entity entity,
	const LocomotionState& loco, Transform& trans, const Velocity& vel,
	const float dT)
{
	if (loco.isMoving)
	{
		// TEMP : 걷기 뛰기에 따라 속도 조정
		const float speed = 2.0f;

		XMVECTOR pos = XMLoadFloat3(&trans.position);
		XMVECTOR dir = XMLoadFloat3(&vel.dir);

		pos = XMVectorAdd(pos, XMVectorScale(dir, speed * dT));
		XMStoreFloat3(&trans.position, pos);

		float moveYaw = atan2f(
			-XMVectorGetX(dir),
			-XMVectorGetZ(dir)
		);

		XMVECTOR q = XMQuaternionRotationRollPitchYaw(0, moveYaw, 0);
		XMStoreFloat4(&trans.rotation, q);

		Framework::Get().outEventQueue.push(OutputEvent{
			entity, DirtyType::Moved });
	}
}