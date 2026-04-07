#pragma once

#include <span>
#include <unordered_map>
#include <vector>

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

	// dtSec 기준 누적 시간 업데이트
	void FlushLifecycle(double dtSec);
	void CollectDestroyable();

	// [ admission / transfer accounting ]
	bool AddReservedSlots(WorldId worldId, uint32_t slots);
	bool RemoveReservedSlots(WorldId worldId, uint32_t slots);

	bool AddInflightTransferIn(WorldId worldId, uint32_t slots);
	bool RemoveInflightTransferIn(WorldId worldId, uint32_t slots);

	bool AddInflightTransferOut(WorldId worldId, uint32_t slots);
	bool RemoveInflightTransferOut(WorldId worldId, uint32_t slots);

	std::span<const WorldId> GetRunnableWorldIds();

	const WorldRegistry& GetRegistry() const { return _registry; }

private:
	void MarkRunnableWorldIdsDirty() noexcept;
	void RebuildRunnableWorldIds();

	static void FillRecordFromDef(
		WorldInstanceRecord& record,
		const WorldDef& def,
		uint64_t normalizedInstanceKey
	);

	bool TryFindAliveResolvedWorld(
		const WorldResolveKey& key,
		WorldId& outWorldId
	);

	WorldId CreateAndRegisterWorld(
		const WorldDef& def,
		uint64_t normalizedInstanceKey,
		const WorldResolveKey& key
	);

	static WorldResolveKey MakeRecordResolveKey(const WorldInstanceRecord& record);

	static bool HasClosingIntent(const WorldInstanceRecord& record);
	static bool IsDestroySafe(const WorldInstanceRecord& record);
	static bool ShouldStartClosing(const WorldInstanceRecord& record);
	static bool ShouldEnterDestroyPending(const WorldInstanceRecord& record);

	static void UpdateEmptyElapsed(WorldInstanceRecord& record, const double dtSec);
	static void UpdateClosingElapsed(WorldInstanceRecord& record, const double dtSec);
	static void ResetLifecycleAccumulatorsOnActivity(WorldInstanceRecord& record);


private:
	WorldRegistry& _registry;
	std::unordered_map<WorldId, WorldInstanceRecord> _records;
	std::unordered_map<WorldResolveKey, WorldId> _resolveIndex;

	std::vector<WorldId> _runnableWorldIds;
	bool _runnableWorldIdsDirty{ true };
};
