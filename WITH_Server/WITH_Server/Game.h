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

	WorldId CreateWorld(const WorldDesc& desc, uint32 tickRate);
	void DestroyWorld(WorldId worldId);

	void PauseWorld(WorldId worldId);
	void ResumeWorld(WorldId worldId);

	void Update(const double dT);
	void Stop();

private:
	void DestroyAllWorlds();

	bool _running;
	Config _cfg;

	ThreadPool _pool;
	IWorldFactory& _factory;

	WorldRegistry _registry;
	WorldScheduler _scheduler;

	std::vector<WorldId> _ownedWorldIds;
};
