#include "pch.h"
#include "ServerConnectionListener.h"

#include "Connection.h"
#include "Framework.h"

ServerConnectionListener::ServerConnectionListener()
	: _sendBuffers(*this)
{
}

void ServerConnectionListener::OnConnected(Connection& conn)
{
	_connRegistry.Add(conn.shared_from_this());
	_idMap.OnConnected(conn.GetId());
}

void ServerConnectionListener::OnDisconnected(Connection& conn)
{
	const uint32 id = conn.GetId();
	const NetId netId = _idMap.GetPlayer(id);
	const Entity entity = Framework::Get().netIdRegistry.FindEntity(netId);

	DisconnectEvent dc{ id, netId, entity };
	Event ev{ EventType::EV_DISCONNECT, dc };
	Framework::Get().eventQueue.push(ev);

	_sendBuffers.ReleaseSession(id);
	_connRegistry.Remove(id);
	_idMap.OnDisconnected(id);
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
	_connRegistry.Send(id, data);
}

void ServerConnectionListener::Broadcast(SendBuffer* data, uint32 expected)
{
	_connRegistry.Broadcast(data, expected);
}
