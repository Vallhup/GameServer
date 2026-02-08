#include "pch.h"
#include "NetIdRegistry.h"

NetIdRegistry::NetIdRegistry(uint32 reserve)
{
	_entities.reserve(reserve);
	_entities.push_back(Entity::Null());
}

NetId NetIdRegistry::Allocate()
{
	NetId id = _allocator.Allocate();
	EnsureCapacity(id.GetId());
	return id;
}

void NetIdRegistry::Free(NetId netId)
{
	if (!netId.IsValid()) return;

	const uint32 id = netId.GetId();
	if (id == 0 || id >= _entities.size()) return;

	if (_allocator.IsAlive(netId))
		_entities[id] = Entity::Null();

	_allocator.Free(netId);
}

bool NetIdRegistry::BindEntity(NetId netId, Entity entity)
{
	if (!netId.IsValid()) return false;
	if (entity.IsNull()) return false;
	if (!_allocator.IsAlive(netId)) return false;

	const uint32 id = netId.GetId();
	EnsureCapacity(id);

	_entities[id] = entity;
	return true;
}

bool NetIdRegistry::UnbindEntity(NetId netId)
{
	if (!netId.IsValid()) return false;
	if (!_allocator.IsAlive(netId)) return false;

	const uint32 id = netId.GetId();
	if (id == 0 || id >= _entities.size())
		return false;

	_entities[id] = Entity::Null();
	return true;
}

Entity NetIdRegistry::FindEntity(NetId netId) const
{
	if (!netId.IsValid()) return Entity::Null();
	if (!_allocator.IsAlive(netId)) return Entity::Null();

	const uint32 id = netId.GetId();
	if (id == 0 || id >= _entities.size())
		return Entity::Null();

	return _entities[id];
}

bool NetIdRegistry::IsAlive(NetId netId) const
{
	return _allocator.IsAlive(netId);
}

void NetIdRegistry::EnsureCapacity(uint32 netId)
{
	if (netId < _entities.size()) return;
	_entities.resize(netId + 1);
}
