#include "pch.h"
#include "Network.h"
#include "Connection.h"
#include "IConnectionListener.h"

Network::Network(uint16 threadCnt, uint16 port, IConnectionListener& connListener)
	: ServerService(threadCnt, port, connListener)
{
}

void Network::OnAccept(tcp::socket s)
{
	uint32 id = _nextId.fetch_add(1);

	auto conn = std::make_shared<Connection>(std::move(s), _connListener);
	conn->SetId(id);
	conn->Start();
}
