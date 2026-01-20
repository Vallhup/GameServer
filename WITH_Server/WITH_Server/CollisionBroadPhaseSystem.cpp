#include "pch.h"
#include "CollisionBroadPhaseSystem.h"
#include "Collision.h"

void CollisionBroadPhaseSystem::Execute(const float dT)
{
	ecs.mapCollisionPairs.clear();
	
	// TEMP : 맵 충돌 Box 배열이 비어있으면 return;
	if (ecs.mapOBBs.empty()) return;

	auto& colliders = ecs.GetStorage<MapCollider>();

	for (const auto& [entity, collider] : colliders)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		const AABB& aabb = collider.worldBodyAABB;

		for (const auto& obb : ecs.mapOBBs)
		{
			if (!Collision::CheckAABBVsAABB(aabb, obb.aabb)) continue;
			ecs.mapCollisionPairs.emplace_back(entity, obb.id);
		}
	}
}