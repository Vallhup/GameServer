#pragma once

class Client;

class IScenario {
public:
	virtual ~IScenario() = default;

public:
	virtual void OnStart(Client& client) = 0;
	virtual void OnTick(Client& client) = 0;
	virtual void OnPacket(Client& client, const unsigned char* buf) = 0;
};

