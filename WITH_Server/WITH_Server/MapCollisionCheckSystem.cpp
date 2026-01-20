#include "pch.h"
#include "MapCollisionCheckSystem.h"
#include "Collision.h"

void MapCollisionCheckSystem::Execute(const float dT)
{
	auto& mapPairs = ecs.mapCollisionPairs;
	auto& colliders = ecs.GetStorage<MapCollider>();

	for (const auto& pair : mapPairs)
	{
		auto* collider = colliders.GetComponent(pair.entity);
		if (!collider) continue;

		const OBB& obb = ecs.mapOBBs[pair.obbId];
		CapsuleView capsule = MakeCapsuleView(*collider);

		XMFLOAT3 normal{ 0, 0, 0 };
		float pen{ 0.0f };
		if (Collision::CheckOBBVsCapsule(obb, capsule, &normal, &pen))
		{
			ecs.mapCollisionEvents.emplace_back(
				pair.entity, pair.obbId, normal, pen);
		}
	}
}