#pragma once

#include "WorldId.h"

class WorldRegistry;
class WorldService;
class WorldLeapManager;

class WorldScheduler {
public:
	explicit WorldScheduler(WorldRegistry& reg, WorldService& service, WorldLeapManager& leapMng) 
		: _reg(reg), _service(service), _leapMng(leapMng){}

	void Register(WorldId id, uint32 tickRate);
	void Unregister(WorldId id);

	void Update(const double dT);

	void Clear();

private:
	struct Entry 
	{
		WorldId id;
		double tickInterval{ 0.016 };
		double acc{ 0.0 };
		bool paused{ false };
	};

	WorldRegistry& _reg;
	WorldService& _service;
	WorldLeapManager& _leapMng;

	std::vector<Entry> _entries;
};

static inline double TickIntervalFromRate(uint32 tickRate)
{
	if (tickRate == 0) tickRate = 1;
	return 1.0 / double(tickRate);
}

