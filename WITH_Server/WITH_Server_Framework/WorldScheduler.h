#pragma once

#include "WorldId.h"

class WorldRegistry;

class WorldScheduler {
public:
	explicit WorldScheduler(WorldRegistry& reg) : _reg(reg) {}

	void Register(WorldId id, uint32 tickRate);
	void Unregister(WorldId id);

	void Update(const double dT);

private:
	struct Entry 
	{
		WorldId id;
		double tickInterval{ 0.016 };
		double acc{ 0.0 };
		bool paused{ false };
	};

	WorldRegistry& _reg;
	std::vector<Entry> _entries;
};

static inline double TickIntervalFromRate(uint32 tickRate)
{
	if (tickRate == 0) tickRate = 1;
	return 1.0 / double(tickRate);
}

