#pragma once

#include "Client.h"

class IScenario {
public:
	virtual ~IScenario() = default;

public:
	virtual void OnStart(Client* client) = 0;
	virtual void OnTick(Client* client) = 0;
	virtual void OnPacket(Client* client, const std::vector<char>& packet) = 0;
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
	virtual void OnPacket(Client* client, const std::vector<char>& packet) override;
};