#pragma once

#include "IConnectionListener.h"
#include "ConnectionRegistry.h"
#include "NetworkHandler.h"
#include "NetIdMap.h"
#include "Network.h"

class ServerConnectionListener : public IConnectionListener {
public:
	virtual ~ServerConnectionListener() = default;

	virtual void OnConnected(Connection& conn) override;
	virtual void OnDisconnected(Connection& conn) override;
	virtual void OnPacketReceived(Connection& conn, 
		const PacketHeader& header, const BYTE* data) override;

	void EnsurePlayerBound(uint32 connId, WorldId world);

	// TEMP
	void Send(uint32 id, SendBuffer* data);
	void Broadcast(SendBuffer* data, 
		uint32 expected = std::numeric_limits<uint32>::max());

private:
	NetworkHandler _handler;

	NetIdMap _idMap;
	ConnectionRegistry _connRegistry;
};