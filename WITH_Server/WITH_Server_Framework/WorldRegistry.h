#pragma once
#pragma once

#include <memory>
#include <unordered_map>

#include "WorldDef.h"
#include "WorldId.h"
#include "WorldIdAllocator.h"
#include "WorldInstance.h"
#include "IWorldInstanceFactory.h"

class WorldRegistry final {
public:
	WorldRegistry(IWorldInstanceFactory& factory);

	const WorldDef* FindWorldDef(WorldDefId defId) const;
	void RegisterWorldDef(const WorldDef& def);

	WorldInstance* FindWorld(WorldId worldId);
	const WorldInstance* FindWorld(WorldId worldId) const;

	WorldInstance* CreateWorld(const WorldDef& def, uint64_t instanceKey);

	bool DestroyWorld(WorldId worldId);

	bool IsAlive(WorldId worldId) const;

	void Clear();

private:
	IWorldInstanceFactory& _factory;

	WorldIdAllocator _idAllocator;
	std::unordered_map<WorldDefId, WorldDef> _defs;
	std::unordered_map<WorldId, std::unique_ptr<WorldInstance>> _worlds;
};