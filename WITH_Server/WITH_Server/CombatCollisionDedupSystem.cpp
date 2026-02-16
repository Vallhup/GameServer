#include "pch.h"
#include "CombatCollisionDedupSystem.h"

void CombatCollisionDedupSystem::Execute(const double dT)
{
	auto& eventQ = _runtime.Events().Queue<CombatCollisionEvent>();
	auto evView = eventQ.ConsumeView();
	auto events = DedupCollisionEvent(evView);
	

	struct CollisionKey 
	{ 
		Entity a; Entity v; uint32 id; 

		bool operator==(const CollisionKey& other) const noexcept
		{
			return a == other.a && v == other.v && id == other.id;
		}
	};
	
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
			const CollisionKey key{ a, v, id };
			return std::find(clashKeys.begin(), clashKeys.end(), key) != clashKeys.end();
		};

	size_t w{ 0 };
	for (size_t i = 0; i < events.size(); ++i)
	{
		const auto& ev = events[i];
		if (ev.type == CollisionType::Strike &&
			HasClash(ev.attacker, ev.victim, ev.attackId)) continue;

		events[w++] = ev;
	}

	eventQ.ReadResize(w);
}

std::span<CombatCollisionEvent> CombatCollisionDedupSystem::DedupCollisionEvent(std::span<CombatCollisionEvent> events)
{
	if (events.empty()) return events;

	std::sort(events.begin(), events.end(),
		[](const CombatCollisionEvent& a, const CombatCollisionEvent& b)
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

	size_t w{ 0 };
	for (size_t i = 0; i < events.size(); ++i)
	{
		if (w == 0)
		{
			events[w++] = events[i];
			continue;
		}

		const auto& prev = events[w - 1];
		const auto& cur = events[i];

		const bool equalEvent = 
			cur.type		== prev.type &&
			cur.attacker	== prev.attacker &&
			cur.attackId	== prev.attackId &&
			cur.aIndex		== prev.aIndex &&
			cur.bIndex		== prev.bIndex;

		if (!equalEvent)
			events[w++] = cur;
	}

	return events.first(w);
}
