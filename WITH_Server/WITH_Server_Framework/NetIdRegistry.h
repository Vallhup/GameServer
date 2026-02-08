#pragma once

#include "NetId.h"
#include "NetIdAllocator.h"

class NetIdRegistry {
public:
	explicit NetIdRegistry(uint32 reserve = 1024);

	NetId Allocate();
	void Free(NetId netId);

	bool BindEntity(NetId netId, Entity entity);
	bool UnbindEntity(NetId netId);

	Entity FindEntity(NetId netId) const;
	bool IsAlive(NetId netId) const;

private:
	void EnsureCapacity(uint32 netId);

	NetIdAllocator _allocator;
	std::vector<Entity> _entities;
};

