#include "pch.h"
#include "ServerConnectionListener.h"

#include "Connection.h"
#include "Framework.h"

void ServerConnectionListener::OnConnected(Connection& conn)
{
	_connRegistry.Add(conn.shared_from_this());
	_idMap.OnConnected(conn.GetId());
}

void ServerConnectionListener::OnDisconnected(Connection& conn)
{
	uint32 id = conn.GetId();

	DisconnectEvent dc{ id };
	Event ev{ EventType::EV_DISCONNECT, dc };
	Framework::Get().eventQueue.push(ev);

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

void ServerConnectionListener::EnsurePlayerBound(uint32 connId, WorldId world)
{
	if (_idMap.GetPlayer(connId).IsValid())
		return;

	NetId id = Framework::Get().netIdRegistry.Allocate();
	_idMap.BindPlayer(connId, id);
	_idMap.EnterWorld(connId, world);
}

void ServerConnectionListener::Send(uint32 id, SendBuffer* data)
{
	_connRegistry.Send(id, data);
}

void ServerConnectionListener::Broadcast(SendBuffer* data, uint32 expected)
{
	_connRegistry.Broadcast(data, expected);
}
