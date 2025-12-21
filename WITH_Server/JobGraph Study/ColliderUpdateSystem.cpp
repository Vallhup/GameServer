#include "ColliderUpdateSystem.h"
#include "Math.h"

void ColliderUpdateSystem::Execute(const float dT)
{
	auto& transforms = ecs.GetStorage<Transform>();
	auto& colliders = ecs.GetStorage<Collider>();

	for (const auto& [entity, collider] : colliders)
	{
		auto* trans = transforms.GetComponent(entity);
		if (!trans) continue;

		XMMATRIX world = TransformHelper::ToMatrix(*trans);

		collider.worldCapsules.resize(collider.localCapsules.size());
		for (size_t i = 0; i < collider.localCapsules.size(); ++i)
		{
			const Capsule& localCapsule = collider.localCapsules[i];
			Capsule worldCapsule = localCapsule;

			XMVECTOR p0 = XMVector3Transform(localCapsule.P0() * 0.01f, world);
			XMVECTOR p1 = XMVector3Transform(localCapsule.P1() * 0.01f, world);

			XMStoreFloat3(&worldCapsule.p0, p0);
			XMStoreFloat3(&worldCapsule.p1, p1);

			worldCapsule.radius *= 0.01f;
			collider.worldCapsules[i] = worldCapsule;

			/*if (i == 0)
				printf("Entity %d, LocalCapsule : P0(%f, %f, %f), P1(%f, %f, %f), Radius(%f) --> WorldCapsule : P0(%f, %f, %f), P1(%f, %f, %f), Radius(%f)\n",
					entity,
					localCapsule.p0.x, localCapsule.p0.y, localCapsule.p0.z,
					localCapsule.p1.x, localCapsule.p1.y, localCapsule.p1.z,
					localCapsule.radius,
					worldCapsule.p0.x, worldCapsule.p0.y, worldCapsule.p0.z,
					worldCapsule.p1.x, worldCapsule.p1.y, worldCapsule.p1.z,
					worldCapsule.radius);*/
		}
	}
}