#include "pch.h"
#include "NetIdRegistry.h"

NetIdRegistry::NetIdRegistry(uint32 reserve)
	: _allocator(reserve)
{
	_locations.reserve(reserve + 1);
	_locations.push_back(NetBindingLocation{});
}

NetId NetIdRegistry::Allocate()
{
	NetId netId = _allocator.Allocate();
	EnsureCapacity(netId.GetId());
	return netId;
}

void NetIdRegistry::Free(NetId netId)
{
	if (!netId.IsValid())
	{
		return;
	}

	const uint32 id = netId.GetId();
	if (id == 0 || id >= _locations.size())
	{
		return;
	}

	if (_allocator.IsAlive(netId))
	{
		const NetBindingLocation location = _locations[id];
		if (location.IsValid())
		{
			_netIdByWorldEntity.erase(WorldEntityKey{ location.worldId, location.entity });
		}

		_locations[id] = NetBindingLocation{};
	}

	_allocator.Free(netId);
}

bool NetIdRegistry::BindEntity(NetId netId, WorldId worldId, Entity entity)
{
	if (!netId.IsValid())
	{
		return false;
	}

	if (!worldId.IsValid() || entity.IsNull())
	{
		return false;
	}

	if (!_allocator.IsAlive(netId))
	{
		return false;
	}

	const uint32 id = netId.GetId();
	EnsureCapacity(id);

	const NetBindingLocation previous = _locations[id];
	if (previous.IsValid())
	{
		_netIdByWorldEntity.erase(WorldEntityKey{ previous.worldId, previous.entity });
	}

	const WorldEntityKey key{ worldId, entity };

	auto existing = _netIdByWorldEntity.find(key);
	if (existing != _netIdByWorldEntity.end() && existing->second != netId)
	{
		const uint32 existingId = existing->second.GetId();
		if (existingId > 0 && existingId < _locations.size())
		{
			_locations[existingId] = NetBindingLocation{};
		}
	}

	_locations[id] = NetBindingLocation{ worldId, entity };
	_netIdByWorldEntity[key] = netId;
	return true;
}

bool NetIdRegistry::BindEntity(NetId netId, Entity entity)
{
	if (!netId.IsValid())
	{
		return false;
	}

	if (entity.IsNull())
	{
		return false;
	}

	if (!_allocator.IsAlive(netId))
	{
		return false;
	}

	const uint32 id = netId.GetId();
	EnsureCapacity(id);

	const NetBindingLocation previous = _locations[id];
	if (previous.IsValid())
	{
		_netIdByWorldEntity.erase(WorldEntityKey{ previous.worldId, previous.entity });
	}

	_locations[id] = NetBindingLocation{ WorldId::Invalid(), entity };
	return true;
}

bool NetIdRegistry::UnbindEntity(NetId netId)
{
	if (!netId.IsValid())
	{
		return false;
	}

	if (!_allocator.IsAlive(netId))
	{
		return false;
	}

	const uint32 id = netId.GetId();
	if (id == 0 || id >= _locations.size())
	{
		return false;
	}

	const NetBindingLocation location = _locations[id];
	if (location.IsValid())
	{
		_netIdByWorldEntity.erase(WorldEntityKey{ location.worldId, location.entity });
	}

	_locations[id] = NetBindingLocation{};
	return true;
}

NetBindingLocation NetIdRegistry::FindLocation(NetId netId) const
{
	if (!netId.IsValid() || !_allocator.IsAlive(netId))
	{
		return NetBindingLocation{};
	}

	const uint32 id = netId.GetId();
	if (id == 0 || id >= _locations.size())
	{
		return NetBindingLocation{};
	}

	return _locations[id];
}

NetId NetIdRegistry::FindNetId(WorldId worldId, Entity entity) const
{
	if (!worldId.IsValid() || entity.IsNull())
	{
		return NetId::Invalid();
	}

	const auto it = _netIdByWorldEntity.find(WorldEntityKey{ worldId, entity });
	if (it == _netIdByWorldEntity.end())
	{
		return NetId::Invalid();
	}

	if (!_allocator.IsAlive(it->second))
	{
		return NetId::Invalid();
	}

	return it->second;
}

Entity NetIdRegistry::FindEntity(NetId netId) const
{
	return FindLocation(netId).entity;
}

bool NetIdRegistry::IsAlive(NetId netId) const
{
	return _allocator.IsAlive(netId);
}

void NetIdRegistry::EnsureCapacity(uint32 netIdValue)
{
	if (netIdValue < _locations.size())
	{
		return;
	}

	_locations.resize(netIdValue + 1);
}
