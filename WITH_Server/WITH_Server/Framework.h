#pragma once

#include "Game.h"
#include "Network.h"
#include "ServerConnectionListener.h"
#include "NetIdRegistry.h"
#include "TestWorldFactory.h"

class Framework {
public:
	static Framework& Get()
	{
		static Framework framework;
		return framework;
	}

	Framework(const Game::Config& cfg = { 4, 64 });

	void Start();
	void Stop();

	concurrency::concurrent_queue<Event> eventQueue;
	concurrency::concurrent_queue<LifecycleEvent> lifecycleEventQueue;

	NetIdRegistry netIdRegistry;

	TestWorldFactory factory;
	ServerConnectionListener listener;
	Network network;
	Game game;

private:
	static BOOL WINAPI ConsoleHandler(DWORD ctrlType);
	void LoadAnimations();
	void LoadMapDatas();

	bool _running;
};