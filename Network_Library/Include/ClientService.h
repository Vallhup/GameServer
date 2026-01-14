#pragma once

#include "Connection.h"
#include "NetworkService.h"

class IConnectionListener;

class ClientService : public NetworkService {
public:
	explicit ClientService(uint16 threadCnt, std::string_view ip, uint16 port,
		IConnectionListener& listener);
	virtual ~ClientService() = default;
	
	void Send(SendBuffer* data);

protected:
	virtual void OnStart() override;
	virtual void OnStop() override;

private:
	std::string _ip;
	uint16 _port;
	IConnectionListener& _listener;
	std::shared_ptr<Connection> _connection;
};