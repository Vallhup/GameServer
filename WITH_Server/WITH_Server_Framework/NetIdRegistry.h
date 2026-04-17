#pragma once

#include <unordered_map>
#include <vector>

#include "Entity.h"
#include "NetId.h"
#include "NetIdAllocator.h"
#include "WorldId.h"

struct NetBindingLocation
{
	WorldId worldId{ WorldId::Invalid() };
	Entity entity{ Entity::Null() };

	bool IsValid() const noexcept
	{
		return worldId.IsValid() && !entity.IsNull();
	}
};

struct WorldEntityKey
{
	WorldId worldId{ WorldId::Invalid() };
	Entity entity{ Entity::Null() };

	bool IsValid() const noexcept
	{
		return worldId.IsValid() && !entity.IsNull();
	}

	bool operator==(const WorldEntityKey& rhs) const noexcept
	{
		return worldId == rhs.worldId && entity == rhs.entity;
	}
};

namespace std
{
	template<>
	struct hash<WorldEntityKey>
	{
		size_t operator()(const WorldEntityKey& key) const noexcept
		{
			const size_t h1 = std::hash<WorldId>()(key.worldId);
			const size_t h2 = std::hash<Entity>()(key.entity);

			// boost::hash_combine 방식
			return h1 ^ (h2 + 0x9e3779b97f4a7c15ull + (h1 << 6) + (h1 >> 2));
		}
	};
}

class NetIdRegistry {
public:
	explicit NetIdRegistry(uint32 reserve = 1024);

	NetId Allocate();
	void Free(NetId netId);

	bool BindEntity(NetId netId, WorldId worldId, Entity entity);
	bool BindEntity(NetId netId, Entity entity);
	bool UnbindEntity(NetId netId);

	NetBindingLocation FindLocation(NetId netId) const;
	NetId FindNetId(WorldId worldId, Entity entity) const;

	// Convenience accessor for legacy call sites that only need the bound entity.
	Entity FindEntity(NetId netId) const;

	bool IsAlive(NetId netId) const;

private:
	void EnsureCapacity(uint32 netIdValue);

private:
	NetIdAllocator _allocator;
	std::vector<NetBindingLocation> _locations;
	std::unordered_map<WorldEntityKey, NetId> _netIdByWorldEntity;
};
