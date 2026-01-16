#include "pch.h"
#include "MovementApplySystem.h"
#include "Framework.h"

void MovementApplySystem::Execute(const float dT)
{
	auto& transforms = ecs.GetStorage<Transform>();
	auto& aDeltas = ecs.GetStorage<ActionMoveDelta>();
	auto& lDeltas = ecs.GetStorage<LocomotionMoveDelta>();

	for (const auto& [entity, transform] : transforms)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		auto* aDelta = aDeltas.GetComponent(entity);
		auto* lDelta = lDeltas.GetComponent(entity);
		if (!aDelta || !lDelta) continue;

		MovementApply(entity, &transform, aDelta, lDelta, dT);
	}
}

void MovementApplySystem::MovementApply(Entity entity, Transform* trans, 
	ActionMoveDelta* aDelta, LocomotionMoveDelta* lDelta, const float dT)
{
	XMFLOAT3 totalMoveDelta{ 0, 0, 0 };
	bool moved{ false };

	if (aDelta->hasMove)
	{
		totalMoveDelta = aDelta->deltaPos;
		moved = true;

		if (aDelta->hasYaw)
		{
			XMVECTOR q = XMQuaternionRotationRollPitchYaw(
				0.0f, aDelta->yaw, 0.0f);
			XMStoreFloat4(&trans->rotation, q);
		}
	}

	else if (lDelta->hasMove)
	{
		totalMoveDelta = lDelta->deltaPos;
		moved = true;

		if (lDelta->hasYaw)
		{
			XMVECTOR q = XMQuaternionRotationRollPitchYaw(
				0.0f, lDelta->yaw, 0.0f);
			XMStoreFloat4(&trans->rotation, q);
		}
	}

	aDelta->hasMove = false; aDelta->hasYaw = false;
	lDelta->hasMove = false; lDelta->hasYaw = false;

	if (moved)
	{
		XMVECTOR pos = XMLoadFloat3(&trans->position);
		XMVECTOR delta = XMLoadFloat3(&totalMoveDelta);

		pos = XMVectorAdd(pos, delta);
		XMStoreFloat3(&trans->position, pos);

		Framework::Get().outEventQueue.push(OutputEvent{
			entity, DirtyType::Moved });
	}
}
