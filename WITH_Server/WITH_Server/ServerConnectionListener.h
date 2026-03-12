#pragma once

#include "SessionSendBufferManager.h"
#include "IConnectionListener.h"
#include "ConnectionRegistry.h"
#include "NetworkHandler.h"
#include "NetIdMap.h"
#include "Network.h"

class ServerConnectionListener : public IConnectionListener {
public:
	ServerConnectionListener();
	virtual ~ServerConnectionListener() = default;

	virtual void OnConnected(Connection& conn) override;
	virtual void OnDisconnected(Connection& conn) override;
	virtual void OnPacketReceived(Connection& conn, 
		const PacketHeader& header, const BYTE* data) override;

	NetIdMap& GetIdMap() { return _idMap; }
	const NetIdMap& GetIdMap() const { return _idMap; }

	ConnectionRegistry& GetConnRegistry() { return _connRegistry; }
	const ConnectionRegistry& GetConnRegistry() const { return _connRegistry; }

	SessionSendBufferManager& SendBuffers() { return _sendBuffers; }

	void Send(uint32 id, SendBuffer* data);
	void Broadcast(SendBuffer* data, 
		uint32 expected = std::numeric_limits<uint32>::max());

private:
	NetworkHandler _handler;

	NetIdMap _idMap;
	ConnectionRegistry _connRegistry;

	SessionSendBufferManager _sendBuffers;
};