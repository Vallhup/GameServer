#include "ECS.h"
#include "Framework.h"
#include "MovementSystem.h"

#include "Protocol.hpp"
#include "Protocols/Protocol.pb.h"

void MovementSystem::Execute(const float dT)
{
	auto& transforms = ecs.GetStorage<Transform>();
	auto& velocitys = ecs.GetStorage<Velocity>();
	auto& locos = ecs.GetStorage<LocomotionState>();
	auto& framework = Framework::Get();

	for (const auto& [entity, transform] : transforms)
	{
		if (auto* vel = velocitys.GetComponent(entity))
		{
			if (auto* loco = locos.GetComponent(entity))
			{
				if (loco->isMoving)
				{
					// TEMP : 걷기 뛰기에 따라 속도 조정
					const float speed = 2.0f;

					XMVECTOR pos = XMLoadFloat3(&transform.position);
					XMVECTOR dir = XMLoadFloat3(&vel->dir);

					pos = XMVectorAdd(pos, XMVectorScale(dir, speed * dT));
					XMStoreFloat3(&transform.position, pos);

					float moveYaw = atan2f(
						-XMVectorGetX(dir),
						-XMVectorGetZ(dir)
					);

					XMVECTOR q = XMQuaternionRotationRollPitchYaw(0, moveYaw, 0);
					XMStoreFloat4(&transform.rotation, q);

					Framework::Get().outEventQueue.push(OutputEvent{
						entity, DirtyType::Moved });
				}
			}
		}
	}
}