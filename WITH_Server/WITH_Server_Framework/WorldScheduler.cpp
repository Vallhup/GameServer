#include "pch.h"
#include "WorldScheduler.h"
#include "World.h"
#include "WorldRegistry.h"

void WorldScheduler::Register(WorldId id, uint32 tickRate)
{
	auto it = std::find_if(_entries.begin(), _entries.end(),
		[&](const Entry& e)
		{
			return e.id == id;
		});

	if (it != _entries.end())
	{
		it->tickInterval = TickIntervalFromRate(tickRate);
		it->acc = 0.0;
		it->paused = false;
	}

	else
	{
		Entry entry;
		entry.id = id;
		entry.tickInterval = TickIntervalFromRate(tickRate);
		entry.acc = 0.0;
		entry.paused = false;

		_entries.push_back(entry);
	}
}

void WorldScheduler::Unregister(WorldId id)
{
	auto it = std::find_if(_entries.begin(), _entries.end(),
		[&](const Entry& e) { return e.id == id; });

	if (it != _entries.end())
		_entries.erase(it);
}

void WorldScheduler::Update(const double dT)
{
	const double clampDT = std::clamp<double>(dT, 0.0, 0.25);

	for (Entry& entry : _entries)
	{
		if (entry.paused) continue;

		entry.acc += clampDT;

		if (entry.acc >= entry.tickInterval)
		{
			IWorld* world = _reg.GetWorld(entry.id);
			if (!world)
			{
				entry.paused = true;
				continue;
			}

			world->Update(dT);
			entry.acc =
				std::max<double>(0.0, entry.acc - entry.tickInterval);
		}
	}

	_entries.erase(std::remove_if(_entries.begin(), _entries.end(),
		[](const Entry& e) { return e.paused; }), _entries.end());
}
