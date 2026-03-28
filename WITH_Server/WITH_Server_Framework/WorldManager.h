#pragma once

#include <unordered_map>

#include "WorldId.h"
#include "WorldResolveKey.h"
#include "WorldContentIds.h"
#include "WorldInstanceRecord.h"

class WorldRegistry;

class WorldManager {
public:
	explicit WorldManager(WorldRegistry& registry);

	WorldId ResolveOrCreate(WorldDefId worldDefId, uint64_t instanceKey);
	WorldId RegisterPreCreatedWorld(WorldDefId worldDefId, uint64_t instanceKey);

	WorldInstanceRecord* FindRecord(WorldId worldId);
	const WorldInstanceRecord* FindRecord(WorldId worldId) const;

	void RequestClose(WorldId worldId);
	void FlushLifecycle();
	void CollectDestroyable();

	// [ admission / transfer accounting ]
	bool AddReservedSlots(WorldId worldId, uint32_t slots);
	bool RemoveReservedSlots(WorldId worldId, uint32_t slots);

	bool AddInflightTransferIn(WorldId worldId, uint32_t slots);
	bool RemoveInflightTransferIn(WorldId worldId, uint32_t slots);

	bool AddInflightTransferOut(WorldId worldId, uint32_t slots);
	bool RemoveInflightTransferOut(WorldId worldId, uint32_t slots);

private:
	static void FillRecordFromDef(
		WorldInstanceRecord& record,
		const WorldDef& def,
		uint64_t normalizedInstanceKey
	);


	WorldRegistry& _registry;
	std::unordered_map<WorldId, WorldInstanceRecord> _records;
	std::unordered_map<WorldResolveKey, WorldId> _resolveIndex;
};