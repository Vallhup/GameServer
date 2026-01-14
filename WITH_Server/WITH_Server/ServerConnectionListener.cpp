#include "pch.h"
#include "ServerConnectionListener.h"

#include "Connection.h"
#include "Framework.h"

void ServerConnectionListener::OnConnected(Connection& conn)
{
	_connMng.Add(conn);
}

void ServerConnectionListener::OnDisconnected(Connection& conn)
{
	uint32 id = conn.GetId();

	DisconnectEvent dc{ id };
	Event ev{ EventType::EV_DISCONNECT, dc };
	Framework::Get().eventQueue.push(ev);

	_connMng.Remove(conn);
}

void ServerConnectionListener::OnPacketReceived(Connection& conn, const PacketHeader& header, const BYTE* data)
{
	uint32 id = conn.GetId();

	if (id != std::numeric_limits<uint32>::max())
	{
		_handler.Handle(id, header, data);
	}
}

void ServerConnectionListener::Send(uint32 id, SendBuffer* data)
{
	_connMng.Send(id, data);
}

void ServerConnectionListener::Broadcast(SendBuffer* data, uint32 expected)
{
	_connMng.Broadcast(data, expected);
}
