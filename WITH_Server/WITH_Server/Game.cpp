#include "pch.h"
#include "Game.h"
#include <algorithm>

Game::Game(const Config& cfg, IWorldFactory& factory)
	: _running(true), _cfg(cfg), _pool(cfg.workerThreadCnt), _factory(factory),
	_registry(cfg.registryReserve, _pool, _factory), _scheduler(_registry)
{
}

Game::~Game()
{
	Stop();
}

WorldId Game::CreateWorld(const WorldDesc& desc, uint32 tickRate)
{
	const WorldId id = _registry.CreateWorld(desc);
	_scheduler.Register(id, tickRate);
	_ownedWorldIds.push_back(id);
	return id;
}

void Game::DestroyWorld(WorldId worldId)
{
	_scheduler.Unregister(worldId);

	if (auto* world = _registry.GetWorld(worldId))
		world->Shutdown();

	_registry.DestroyWorld(worldId);

	_ownedWorldIds.erase(
		std::remove(_ownedWorldIds.begin(), _ownedWorldIds.end(), worldId), 
		_ownedWorldIds.end());
}

void Game::PauseWorld(WorldId worldId)
{
	// TODO : World Pause/Resume 기능 추가? 고민중
}

void Game::ResumeWorld(WorldId worldId)
{
	// TODO : World Pause/Resume 기능 추가? 고민중
}

void Game::Update(const double dT)
{
	_scheduler.Update(dT);
}

void Game::Stop()
{
	if (_running)
	{
		_running = false;

		DestroyAllWorlds();

		_pool.Stop();
	}
}

void Game::DestroyAllWorlds()
{
	auto ids = _ownedWorldIds;

	for (WorldId id : ids)
		_scheduler.Unregister(id);

	for (WorldId id : ids)
	{
		if (auto* world = _registry.GetWorld(id))
			world->Shutdown();
	}

	for (WorldId id : ids)
		_registry.DestroyWorld(id);

	_ownedWorldIds.clear();
}
