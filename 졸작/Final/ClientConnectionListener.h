#pragma once

#include "IconnectionListener.h"

class ClientConnectionListener : public IConnectionListener {
public:
	virtual ~ClientConnectionListener() = default;

	virtual void OnConnected(Connection& owner) override;
	virtual void OnDisconnected(Connection& owner) override;
	virtual void OnPacketReceived(Connection& owner,
		const PacketHeader& header, const BYTE* data) override;
};

