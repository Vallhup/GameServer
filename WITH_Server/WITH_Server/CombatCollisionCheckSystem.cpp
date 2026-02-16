#include "pch.h"
#include "CombatCollisionCheckSystem.h"
#include "Collision.h"

void CombatCollisionCheckSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	auto& colliders = ecs.GetStorage<CombatCollider>();
	auto events = _runtime.Events().Queue<CombatCollisionEvent>().ConsumeView();

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
		const CombatCollider* colA = colliders.GetComponent(a);
		if (!colA || !colA->staticDatas) continue;

		BuildActiveIndices(*colA, &actA);

		for (size_t j = i + 1; j < entities.size(); ++j)
		{
			Entity b = entities[j];
			const CombatCollider* colB = colliders.GetComponent(b);
			if (!colB || !colB->staticDatas) continue;

			BuildActiveIndices(*colB, &actB);

			CheckCollision(a, *colA, actA, b, *colB, actB);
			CheckCollision(b, *colB, actB, a, *colA, actA);
		}
	}

#ifdef _DEBUG
	if (!events.empty())
		printf("[Collision] events = %zu\n", events.size());
#endif
}

void CombatCollisionCheckSystem::BuildActiveIndices(
	const CombatCollider& collider, ActiveIndices* out)
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

void CombatCollisionCheckSystem::CheckCollision(Entity attacker, 
	const CombatCollider& aCol, const ActiveIndices& aActives, 
	Entity victim, const CombatCollider& vCol, 
	const ActiveIndices& vActives)
{
	if (aActives.offensiveHits.empty()) return;

	auto events = _runtime.Events().Queue<CombatCollisionEvent>();

	CheckCollisionInternal(attacker, aCol, aActives.offensiveHits,
		victim, vCol, vActives.hurts,
		[&](Entity a, Entity v, uint32 atkId, uint16 aOffHit, uint16 vTarget)
		{
			events.Publish({
				CollisionType::Strike,
				a, v, atkId, aOffHit, vTarget });
		});

	CheckCollisionInternal(attacker, aCol, aActives.offensiveHits,
		victim, vCol, vActives.defensiveHits,
		[&](Entity a, Entity v, uint32 atkId, uint16 aOffHit, uint16 vTarget)
		{
			events.Publish({
				CollisionType::Clash,
				a, v, atkId, aOffHit, vTarget });
		});
}

template<typename EmitFunc>
inline void CombatCollisionCheckSystem::CheckCollisionInternal(
	Entity attacker, const CombatCollider& aCol,
	const std::vector<uint16>& aOffHits, Entity victim,
	const CombatCollider& vCol, const std::vector<uint16>& vTargets,
	EmitFunc&& emit)
{
	for (uint16 aOffHit : aOffHits)
	{
		const uint32 atkId = aCol.attackIds[aOffHit];
		const CapsuleView hitCap = MakeCapsuleView(aCol, aOffHit);

		for (uint16 vTarget : vTargets)
		{
			const CapsuleView hurtCap = MakeCapsuleView(vCol, vTarget);

			if (Collision::CheckCapsuleVsCapsule(hitCap, hurtCap))
				emit(attacker, victim, atkId, aOffHit, vTarget);
		}
	}
}