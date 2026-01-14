#pragma once

class Connection;
class PacketHeader;

class IConnectionListener {
public:
	virtual ~IConnectionListener() = default;

	virtual void OnConnected(Connection& owner) = 0;
	virtual void OnDisconnected(Connection& owner) = 0;
	virtual void OnPacketReceived(Connection& owner, 
		const PacketHeader& header, const BYTE* data) = 0;
};