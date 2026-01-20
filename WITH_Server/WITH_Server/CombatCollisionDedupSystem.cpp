#include "pch.h"
#include "CombatCollisionDedupSystem.h"

void CombatCollisionDedupSystem::Execute(const float dT)
{
	auto& events = ecs.collisionEvents;

	if (events.empty()) return;

	// TEMP : Collision Event 정렬, 중복 제거
	std::sort(events.begin(), events.end(),
		[](const CollisionEvent& a, const CollisionEvent& b)
		{
			if (a.type != b.type)
				return static_cast<int>(a.type) < static_cast<int>(b.type);

			if (a.attacker != b.attacker) return a.attacker.id < b.attacker.id;
			if (a.victim != b.victim) return a.victim.id < b.victim.id;
			if (a.attackId != b.attackId) return a.attackId < b.attackId;

			if (a.aIndex != b.aIndex) return a.aIndex < b.aIndex;
			if (a.bIndex != b.bIndex) return a.bIndex < b.bIndex;

			return false;
		}
	);

	events.erase(std::unique(events.begin(), events.end(),
		[](const CollisionEvent& a, const CollisionEvent& b)
		{
			return a.type == b.type &&
				a.attacker == b.attacker &&
				a.attackId == b.attackId &&
				a.aIndex == b.aIndex &&
				a.bIndex == b.bIndex;
		}), events.end());

	struct CollisionKey { Entity a; Entity v; uint32 id; };
	
	std::vector<CollisionKey> clashKeys;
	clashKeys.reserve(events.size());

	for (const auto& event : events)
	{
		if (event.type == CollisionType::Clash)
			clashKeys.emplace_back(event.attacker, event.victim, event.attackId);
	}

	auto HasClash =
		[&](Entity a, Entity v, uint32 id) -> bool
		{
			for (const auto& key : clashKeys)
				if (key.a == a && key.v == v && key.id == id) return true;

			return false;
		};

	events.erase(std::remove_if(events.begin(), events.end(),
		[&](const CollisionEvent& event)
		{
			if (event.type != CollisionType::Strike) return false;
			return HasClash(event.attacker, event.victim, event.attackId);
		}), events.end());
}