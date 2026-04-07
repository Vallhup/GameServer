#include "pch.h"
#include "GameServerNetworkService.h"

#include "Connection.h"

GameServerNetworkService::GameServerNetworkService(
	uint16 workerThreadCount,
	uint16 listenPort,
	uint32 maxPacketSize,
	IConnectionListener& connectionListener,
	IAcceptedConnectionSink& sink)
	: ServerService(workerThreadCount, listenPort, connectionListener)
	, _maxPacketSize(maxPacketSize)
	, _sink(sink)
{
}

asio::any_io_executor GameServerNetworkService::GetExecutor() noexcept
{
	return _ioCtx.get_executor();
}

void GameServerNetworkService::OnAccept(tcp::socket s)
{
	auto connection = std::make_shared<Connection>(
		std::move(s), _connListener, _maxPacketSize);

	_sink.OnAcceptedConnection(connection);
}
