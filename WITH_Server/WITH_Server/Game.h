#pragma once

#include "ThreadPool.h"

class Game final {
public:
	struct Config
	{
		size_t workerThreadCnt{ 4 };
		uint32 registryReserve{ 64 };
	};

	Game(const Config& cfg, IWorldFactory& factory);
	~Game();

	Game(const Game&) = delete;
	Game& operator=(const Game&) = delete;

	bool Init();
	void Update(const double dT);
	void Stop();

private:
	void DestroyAllWorlds();

	bool _running;
	Config _cfg;

	IWorldFactory& _factory;

	ThreadPool _pool;
	WorldRegistry _registry;
	WorldService _service;
	WorldLeapManager _leapMng;
	WorldScheduler _scheduler;

	std::vector<WorldId> _ownedWorldIds;
};
