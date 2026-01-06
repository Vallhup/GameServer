#include "pch.h"
#include "NetworkManager.h"

void NetworkManager::Initialize(uint16 threadCnt, std::string_view ip, uint16 port, 
	IConnectionListener& listener)
{
	if (!_service)
	{
		_service = std::make_unique<ClientService>(threadCnt, ip, port, listener);
		_service->Start();
	}
}

void NetworkManager::Release()
{
	if (_service)
	{
		_service->Stop();
		_service.reset();
		_service = nullptr;
	}
}

void NetworkManager::Send(SendBuffer* packet)
{
	_service->Send(packet);
}
