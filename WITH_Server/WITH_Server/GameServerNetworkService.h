#pragma once

#include <memory>

#include "ServerService.h"

class Connection;
class IConnectionListener;

class IAcceptedConnectionSink {
public:
	virtual ~IAcceptedConnectionSink() = default;

	virtual void OnAcceptedConnection(
		const std::shared_ptr<Connection>& connection) = 0;
};

class GameServerNetworkService final : public ServerService {
public:
	GameServerNetworkService() = delete;
	GameServerNetworkService(
		uint16 workerThreadCount,
		uint16 listenPort,
		uint32 maxPacketSize,
		IConnectionListener& connectionListener,
		IAcceptedConnectionSink& sink);

	virtual ~GameServerNetworkService() = default;

	GameServerNetworkService(const GameServerNetworkService&) = delete;
	GameServerNetworkService& operator=(const GameServerNetworkService&) = delete;

	asio::any_io_executor GetExecutor() noexcept;

public:
	virtual void OnAccept(tcp::socket s) override;

private:
	uint32_t _maxPacketSize{ 0 };
	IAcceptedConnectionSink& _sink;
};
