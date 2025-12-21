#include "ECS.h"
#include "Framework.h"
#include "MovementSystem.h"

#include "Protocol.hpp"
#include "Protocols/Protocol.pb.h"

void MovementSystem::Execute(const float dT)
{
	auto& transforms = ecs.GetStorage<Transform>();
	auto& velocitys = ecs.GetStorage<Velocity>();
	auto& framework = Framework::Get();

	for (const auto& [entity, transform] : transforms)
	{
		if (auto* vel = velocitys.GetComponent(entity))
		{
			int inputX = vel->inputX;
			int inputZ = vel->inputZ;
			float yaw = vel->yaw;

			XMVECTOR forward = XMVectorSet(sin(yaw), 0, cos(yaw), 0);
			XMVECTOR right = XMVector3Cross(XMVectorSet(0, 1, 0, 0), forward);

			XMVECTOR dir = XMVectorAdd(
				XMVectorScale(forward, inputZ),
				XMVectorScale(right, inputX)
			);

			bool moved{ false };
			if (XMVectorGetX(XMVector3LengthSq(dir)) > 1e-6f)
			{
				dir = XMVector3Normalize(dir);
				moved = true;
			}
				
			else
			{
				dir = XMVectorZero();
				moved = false;
			}

			const float speed = 2.0f;
			XMVECTOR pos = XMLoadFloat3(&transform.position);
			pos = XMVectorAdd(pos, XMVectorScale(dir, speed * dT));
			XMStoreFloat3(&transform.position, pos);

			XMVECTOR q = XMQuaternionRotationRollPitchYaw(0, yaw, 0);
			XMStoreFloat4(&transform.rotation, q);

			if (moved)
			{
				Framework::Get().outEventQueue.push(OutputEvent{
					entity, DirtyType::Moved });
			}
		}
	}
}