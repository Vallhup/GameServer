#pragma once

#include <cstdint>

#include "InboundMessage.h"
#include "IConnectionListener.h"

class Connection;
struct PacketHeader;

class IInboundMessageSink {
public:
	virtual ~IInboundMessageSink() = default;

	virtual void OnInboundMessage(InboundMessage&& message) = 0;
};

class GameServerNetworkListener final : public IConnectionListener {
public:
	explicit GameServerNetworkListener(IInboundMessageSink& sink);
	~GameServerNetworkListener() override = default;

	GameServerNetworkListener(const GameServerNetworkListener&) = delete;
	GameServerNetworkListener& operator=(const GameServerNetworkListener&) = delete;

public:
	void OnConnected(Connection& owner) override;
	void OnDisconnected(Connection& owner) override;
	void OnPacketReceived(
		Connection& owner,
		const PacketHeader& header,
		const BYTE* data) override;

private:
	IInboundMessageSink& _sink;
};
