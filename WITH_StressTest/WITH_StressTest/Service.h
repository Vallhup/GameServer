#pragma once

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <WS2tcpip.h>
#include <mswsock.h>
#include <Windows.h>

#include <iostream>
#include <vector>
#include <thread>

#include "Client.h"
#include "ClientManager.h"
#include "IocpCore.h"
#include "Visualizer.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "MSWSock.LIB")

class Service {
public:
	static Service& Instance()
	{
		static Service instance;
		return instance;
	}

public:
	Service();
	
public:
	void Start();
	void Stop();

	void RegisterClient(const std::shared_ptr<Client>& client);

	ClientManager& GetClientManager() const { return *_clientMng; }

private:
	void MainLoop();

private:
	std::unique_ptr<IocpCore> _iocpCore;
	std::unique_ptr<Visualizer> _visualizer;
	std::unique_ptr<ClientManager> _clientMng;

	std::vector<std::thread> _workers;
	std::atomic<bool> _running;
};

