#pragma once

#include "Game.h"
#include "Network.h"
#include <concurrent_queue.h>

class Framework {
public:
	static Framework& Get()
	{
		static Framework framework;
		return framework;
	}

	Framework(size_t size = std::thread::hardware_concurrency());

	void Start();
	void Stop();

	concurrency::concurrent_queue<Event> eventQueue;
	concurrency::concurrent_queue<OutputEvent> outEventQueue;

	std::unordered_map<int, Entity> sessionToEntity;
	std::unordered_map<Entity, int> entityToSession;

	Network network;
	Game game;

private:
	static BOOL WINAPI ConsoleHandler(DWORD ctrlType);

	bool _running;
};