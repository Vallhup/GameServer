#pragma once

#include "NetworkService.h"
#include "ConnectionRegistry.h"

class StressTestClientService : public NetworkService {
public:
	explicit StressTestClientService(uint16 threadCnt, std::string_view ip,
		uint16 port, IConnectionListener& listener);
	virtual ~StressTestClientService() = default;

	void Send(uint32 id, SendBuffer* data);
	void Broadcast(SendBuffer* data, uint32 expected);

protected:
	virtual void OnStart() = 0;
	virtual void OnStop() = 0;

	uint16 _port;
	std::string _ip;
	IConnectionListener& _listener;
	ConnectionRegistry _connRegistry;
};