#pragma once
#include <memory>
#include <unordered_map>

#include "WorldDef.h"
#include "WorldId.h"
#include "WorldIdAllocator.h"
#include "WorldInstance.h"
#include "IWorldInstanceFactory.h"
#include "WorldExecutionModelTypes.h"
#include "WorldTransferProfileRegistry.h"

class WorldRegistry final {
public:
	WorldRegistry(IWorldInstanceFactory& factory);
	WorldRegistry(
		IWorldInstanceFactory& factory,
		WorldExecutionModelRegistry& executionModelRegistry);
	WorldRegistry(
		IWorldInstanceFactory& factory,
		WorldExecutionModelRegistry& executionModelRegistry,
		WorldTransferProfileRegistry& transferProfileRegistry);
	WorldRegistry(
		IWorldInstanceFactory& factory,
		WorldTransferProfileRegistry& transferProfileRegistry);

	const WorldDef* FindWorldDef(WorldDefId defId) const;
	bool RegisterWorldDef(const WorldDef& def);

	WorldInstance* FindWorld(WorldId worldId);
	const WorldInstance* FindWorld(WorldId worldId) const;

	WorldInstance* CreateWorld(const WorldDef& def, uint64_t instanceKey);

	bool DestroyWorld(WorldId worldId);

	bool IsAlive(WorldId worldId) const;

	void Clear();

private:
	IWorldInstanceFactory& _factory;
	WorldExecutionModelRegistry* _executionModelRegistry{ nullptr };
	WorldTransferProfileRegistry* _transferProfileRegistry{ nullptr };

	WorldIdAllocator _idAllocator;
	std::unordered_map<WorldDefId, WorldDef> _defs;
	std::unordered_map<WorldId, std::unique_ptr<WorldInstance>> _worlds;
};

