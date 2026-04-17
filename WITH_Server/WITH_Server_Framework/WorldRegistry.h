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

class IWorldTransferBinding;

class WorldRegistry final {
public:
	WorldRegistry(
		IWorldInstanceFactory& factory,
		const IWorldTransferBinding* transferBinding = nullptr);
	WorldRegistry(
		IWorldInstanceFactory& factory,
		WorldExecutionModelRegistry& executionModelRegistry,
		const IWorldTransferBinding* transferBinding = nullptr);
	WorldRegistry(
		IWorldInstanceFactory& factory,
		WorldExecutionModelRegistry& executionModelRegistry,
		WorldTransferProfileRegistry& transferProfileRegistry,
		const IWorldTransferBinding* transferBinding = nullptr);
	WorldRegistry(
		IWorldInstanceFactory& factory,
		WorldTransferProfileRegistry& transferProfileRegistry,
		const IWorldTransferBinding* transferBinding = nullptr);

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
	const IWorldTransferBinding* _transferBinding{ nullptr };

	WorldIdAllocator _idAllocator;
	std::unordered_map<WorldDefId, WorldDef> _defs;
	std::unordered_map<WorldId, std::unique_ptr<WorldInstance>> _worlds;
};

