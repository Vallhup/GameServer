#include "pch.h"
#include "CollisionCheckSystem.h"
#include "Collision.h"

void CollisionCheckSystem::Execute(const float dT)
{
	auto& colliders = ecs.GetStorage<Collider>();

	ecs.events.clear();
	ecs.events.reserve(128);

	std::vector<Entity> entities;
	entities.reserve(colliders.Size());

	for (const auto& [entity, _] : colliders)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;
		entities.push_back(entity);
	}

	ActiveIndices actA, actB;

	for (size_t i = 0; i < entities.size(); ++i)
	{
		Entity a = entities[i];
		const Collider* colA = colliders.GetComponent(a);
		if (!colA || !colA->staticDatas) continue;

		BuildActiveIndices(*colA, &actA);

		for (size_t j = i + 1; j < entities.size(); ++j)
		{
			Entity b = entities[j];
			const Collider* colB = colliders.GetComponent(b);
			if (!colB || !colB->staticDatas) continue;

			BuildActiveIndices(*colB, &actB);

			CheckCollision(a, *colA, actA, b, *colB, actB);
			CheckCollision(b, *colB, actB, a, *colA, actA);
		}
	}

#ifdef _DEBUG
	if (!ecs.events.empty())
		printf("[Collision] events = %zu\n", ecs.events.size());
#endif
}

CapsuleView CollisionCheckSystem::MakeCapsuleView(const Collider& collider, size_t i)
{
	CapsuleView out;
	out.p0 = collider.worldDatas[i].p0;
	out.p1 = collider.worldDatas[i].p1;
	out.radius = (*collider.staticDatas)[i].radius; 

	return out;
}

void CollisionCheckSystem::BuildActiveIndices(const Collider& collider,
	ActiveIndices* out)
{
	out->offensiveHits.clear();
	out->defensiveHits.clear();
	out->hurts.clear();

	const size_t n = collider.staticDatas->size();
	out->offensiveHits.reserve(n);
	out->defensiveHits.reserve(n);
	out->hurts.reserve(n);

	for (size_t i = 0; i < n; ++i)
	{
		const uint8 enabled = collider.enabledMasks[i];

		if (HasType(enabled, HitboxType::Hurt))
			out->hurts.push_back(static_cast<uint16>(i));

		if (HasType(enabled, HitboxType::Hit))
		{
			if (collider.attackIds[i] != 0)
				out->offensiveHits.push_back(static_cast<uint16>(i));

			else
				out->defensiveHits.push_back(static_cast<uint16>(i));
		}
	}
}

void CollisionCheckSystem::CheckCollision(Entity attacker, const Collider& aCol,
	const ActiveIndices& aActives, Entity victim, const Collider& vCol, 
	const ActiveIndices& vActives)
{
	if (aActives.offensiveHits.empty()) return;

	CheckCollisionInternal(attacker, aCol, aActives.offensiveHits,
		victim, vCol, vActives.hurts,
		[&](Entity a, Entity v, uint32 atkId, uint16 aOffHit, uint16 vTarget)
		{
			ecs.events.push_back({
				CollisionType::Strike,
				a, v, atkId, aOffHit, vTarget });
		});

	CheckCollisionInternal(attacker, aCol, aActives.offensiveHits,
		victim, vCol, vActives.defensiveHits,
		[&](Entity a, Entity v, uint32 atkId, uint16 aOffHit, uint16 vTarget)
		{
			ecs.events.push_back({
				CollisionType::Clash,
				a, v, atkId, aOffHit, vTarget });
		});
}
