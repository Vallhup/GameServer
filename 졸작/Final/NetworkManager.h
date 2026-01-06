#pragma once

#include "ClientService.h"

class NetworkManager {
public:
	NetworkManager() : _service(nullptr) {}

	void Initialize(uint16 threadCnt, std::string_view ip, uint16 port,
		IConnectionListener& listener);
	void Release();

	void Send(SendBuffer* packet);

private:
	std::unique_ptr<ClientService> _service;
};