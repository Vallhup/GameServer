#pragma once

#include <unordered_map>
#include <random>
#include <chrono>

#include "Client.h"

class IScenario {
public:
	virtual ~IScenario() = default;

public:
	virtual void OnStart(Client* client) = 0;
	virtual void OnTick(Client* client) = 0;
};

class ConnectScenario : public IScenario {
public:
	static ConnectScenario& Instance()
	{
		static ConnectScenario instance;
		return instance;
	}

public:
	virtual ~ConnectScenario() = default;

public:
	virtual void OnStart(Client* client) override;
	virtual void OnTick(Client* client) override;
};

class MoveScenario : public IScenario {
public:
	static MoveScenario& Instance()
	{
		static MoveScenario instance;
		return instance;
	}

public:
	virtual ~MoveScenario() = default;

public:
	virtual void OnStart(Client* client) override;
	virtual void OnTick(Client* client) override;

private:
	std::unordered_map<int, std::chrono::high_resolution_clock::time_point> _lastMove;
	std::default_random_engine dre;
};