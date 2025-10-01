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
#include "IocpCore.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "MSWSock.LIB")

class Service {
public:
	Service();
	
public:
	bool Start();
	void Stop();

private:
	std::vector<std::unique_ptr<Client>> _clients;
	std::vector<std::thread> _workers;

	std::unique_ptr<IocpCore> _iocpCore;

	std::atomic<bool> _running;
};

