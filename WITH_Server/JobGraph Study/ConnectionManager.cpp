#include "pch.h"
#include "ConnectionManager.h"

void ConnectionManager::Add(Connection& conn)
{
	uint32 id = conn.GetId();
	auto c = conn.shared_from_this();
	{
		std::lock_guard lock{ _mtx };
		_connections[id] = std::move(c);
	}
}

void ConnectionManager::Remove(Connection& conn)
{
	std::lock_guard lock{ _mtx };

	auto it = _connections.find(conn.GetId());
	if (it != _connections.end())
	{
		_connections.erase(it);
	}
}

Connection* ConnectionManager::GetConnection(uint32 id)
{
	std::lock_guard lock{ _mtx };
	auto it = _connections.find(id);
	if (it != _connections.end())
	{
		return it->second.get();
	}

	return nullptr;
}

void ConnectionManager::Send(uint32 id, SendBuffer* data)
{
	std::lock_guard lock{ _mtx };
	auto it = _connections.find(id);
	if (it != _connections.end())
	{
		it->second->Send(data);
	}
}

void ConnectionManager::Broadcast(SendBuffer* data, uint32 expected)
{
	std::lock_guard lock{ _mtx };
	for (auto& [id, connection] : _connections)
	{
		if (expected == id) continue;
		SendBuffer* copy = data->Clone();
		connection->Send(copy);
	}
}