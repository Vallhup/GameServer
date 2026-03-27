#pragma once

#include <unordered_map>

#include "WorldId.h"
#include "WorldContentIds.h"
#include "WorldInstanceRecord.h"

class WorldRegistry;

class WorldManager {
public:
	explicit WorldManager(WorldRegistry& registry);

	WorldId ResolveOrCreate(WorldDefId worldDefId, uint64_t instanceKey);

	WorldInstanceRecord* FindRecord(WorldId worldId);
	const WorldInstanceRecord* FindRecord(WorldId worldId) const;

	void RequestClose(WorldId worldId);
	void FlushLifecycle();
	void CollectDestroyable();

private:
	WorldRegistry& _registry;
	std::unordered_map<WorldId, WorldInstanceRecord> _records;
};