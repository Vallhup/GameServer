#pragma once

#include "ServerService.h"
#include "Connection.h"

class Network : public ServerService {
public:
	explicit Network(uint16 threadCnt, uint16 port,
		IConnectionListener& connListener);
	virtual ~Network() = default;

	virtual void OnAccept(tcp::socket s) override;

private:
	std::atomic<uint32> _nextId{ 0 };
};