#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <ranges>
#include <shared_mutex>

#include "Client.h"

class Service;

class ClientManager {
public:
	ClientManager() = delete;
	ClientManager(int maxClients, Service& service)
		: _maxClients(maxClients), _service(service), _globalDelay(0) {}
	~ClientManager() = default;

public:
	void AdjustClients();
	void OnTick();
	void DisconnectClient(int id);

	std::vector<Client*> GetClientList();

private:
	std::vector<std::shared_ptr<Client>> _clients;
	
	int _globalDelay;
	int _maxClients;

	Service& _service;
};

