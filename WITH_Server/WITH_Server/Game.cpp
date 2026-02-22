#include "pch.h"
#include "Game.h"
#include <algorithm>

Game::Game(const Config& cfg, IWorldFactory& factory)
	: _running(true), _cfg(cfg), _pool(cfg.workerThreadCnt), _factory(factory),
	_registry(cfg.registryReserve, _pool, _factory), _service(_registry),
	_leapMng(_registry, _service), _scheduler(_registry, _service, _leapMng)
{
	_service.SetScheduler(_scheduler);
}

Game::~Game()
{
	Stop();
}

bool Game::Init()
{
	const bool result = _service.InitSquare();
	assert(result);
	return result;
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
	_leapMng.CommitFrame();
	_leapMng.CommitFrame();
	_service.CommitDestroy();

	_service.Clear();
	_scheduler.Clear();
	_registry.Clear();
}
