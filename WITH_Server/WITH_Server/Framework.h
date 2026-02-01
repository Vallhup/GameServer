#pragma once

#include "Game.h"
#include "Network.h"
#include "ServerConnectionListener.h"

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

	std::unordered_map<uint32, Entity> sessionToEntity;
	std::unordered_map<Entity, uint32> entityToSession;

	ServerConnectionListener listener;
	Network network;
	Game game;

private:
	static BOOL WINAPI ConsoleHandler(DWORD ctrlType);
	void LoadAnimations();
	void LoadMapDatas();

	bool _running;
};