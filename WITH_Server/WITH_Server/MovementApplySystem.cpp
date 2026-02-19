#include "pch.h"
#include "MovementApplySystem.h"
#include "Framework.h"
#include "RepComponent.h"

void MovementApplySystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

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
	ActionMoveDelta* aDelta, LocomotionMoveDelta* lDelta, const double dT)
{
	bool rotated{ false };
	if (aDelta->hasYaw)
	{
		XMVECTOR q = XMQuaternionRotationRollPitchYaw(0.0f, aDelta->yaw, 0.0f);
		XMStoreFloat4(&trans->rotation, q);
		rotated = true;
	}

	else if (lDelta->hasYaw)
	{
		XMVECTOR q = XMQuaternionRotationRollPitchYaw(0.0f, lDelta->yaw, 0.0f);
		XMStoreFloat4(&trans->rotation, q);
		rotated = true;
	}

	XMFLOAT3 totalMoveDelta{ 0, 0, 0 };
	bool moved{ false };

	if (aDelta->hasMove)
	{
		totalMoveDelta = aDelta->deltaPos;
		moved = true;
	}

	else if (lDelta->hasMove)
	{
		totalMoveDelta = lDelta->deltaPos;
		moved = true;
	}

	aDelta->hasMove = false; aDelta->hasYaw = false;
	lDelta->hasMove = false; lDelta->hasYaw = false;

	if (moved)
	{
		totalMoveDelta.y = 0;

		float nx = trans->position.x;
		float nz = trans->position.z;

		if (MapCollisionManager::Get().CanMove(nx + totalMoveDelta.x, nz))
			nx += totalMoveDelta.x;

		if (MapCollisionManager::Get().CanMove(nx, nz + totalMoveDelta.z))
			nz += totalMoveDelta.z;

		if (nx != trans->position.x || nz != trans->position.z)
		{
			trans->position.x = nx;
			trans->position.z = nz;

			trans->position.y =
				MapCollisionManager::Get().SampleHeightAt(trans->position.x, trans->position.z);

			const auto* netComp = _runtime.GetECS().GetStorage<NetIdComp>().GetComponent(entity);
			if (!netComp) return;

			Framework::Get().outEventQueue.push(OutputEvent{netComp->id, DirtyType::Moved});
		}
	}

	if (rotated)
	{
		const auto* netComp = _runtime.GetECS().GetStorage<NetIdComp>().GetComponent(entity);
		if (!netComp) return;

		Framework::Get().outEventQueue.push(OutputEvent{ netComp->id, DirtyType::Moved });
	}
}
