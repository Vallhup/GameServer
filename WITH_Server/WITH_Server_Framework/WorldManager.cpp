#include "pch.h"
#include "WorldManager.h"

#include <vector>

#include "WorldDef.h"
#include "WorldRegistry.h"
#include "WorldResolveRules.h"

WorldManager::WorldManager(WorldRegistry& registry)
	: _registry(registry)
{
}

WorldId WorldManager::ResolveOrCreate(WorldDefId worldDefId, uint64_t instanceKey)
{
	if (worldDefId == WorldDefId::None)
		return WorldId::Invalid();

	const WorldDef* def = _registry.FindWorldDef(worldDefId);
	if (def == nullptr)
		return WorldId::Invalid();

	if (ValidateResolveInput(*def, instanceKey) != WorldResolveValidationResult::Success)
		return WorldId::Invalid();

	const WorldResolveKey key = MakeResolveKey(*def, instanceKey);

	WorldId existingId = WorldId::Invalid();
	if (TryFindAliveResolvedWorld(key, existingId))
		return existingId;

	if (def->entryPolicy.creationPolicy == CreationPolicy::PreCreated)
		return WorldId::Invalid();

	return CreateAndRegisterWorld(*def, key.instanceKey, key);
}

WorldId WorldManager::RegisterPreCreatedWorld(WorldDefId worldDefId, uint64_t instanceKey)
{
	if (worldDefId == WorldDefId::None)
		return WorldId::Invalid();

	const WorldDef* def = _registry.FindWorldDef(worldDefId);
	if (def == nullptr)
		return WorldId::Invalid();

	if (ValidateResolveInput(*def, instanceKey) != WorldResolveValidationResult::Success)
		return WorldId::Invalid();

	const WorldResolveKey key = MakeResolveKey(*def, instanceKey);

	WorldId existingId = WorldId::Invalid();
	if (TryFindAliveResolvedWorld(key, existingId))
		return existingId;

	return CreateAndRegisterWorld(*def, key.instanceKey, key);
}

WorldInstanceRecord* WorldManager::FindRecord(WorldId worldId)
{
	auto it = _records.find(worldId);
	if (it == _records.end())
		return nullptr;

	return &it->second;
}

const WorldInstanceRecord* WorldManager::FindRecord(WorldId worldId) const
{
	auto it = _records.find(worldId);
	if (it == _records.end())
		return nullptr;

	return &it->second;
}

void WorldManager::RequestClose(WorldId worldId)
{
	WorldInstanceRecord* record = FindRecord(worldId);
	if (record == nullptr)
		return;

	record->closeRequested = true;
}

void WorldManager::FlushLifecycle(double dtSec)
{
	if (dtSec < 0.0)
		dtSec = 0.0;

	for (auto& [_, record] : _records)
	{
		switch (record.stage) {
		case WorldStage::Allocated:
		{
			record.stage = WorldStage::Bootstrapping;
			record.emptyElapsedSec = 0.0;
			record.closingElapsedSec = 0.0;
			break;
		}
		case WorldStage::Bootstrapping:
		{
			record.stage = WorldStage::Running;
			record.emptyElapsedSec = 0.0;
			record.closingElapsedSec = 0.0;
			break;
		}
		case WorldStage::Running:
		{
			ResetLifecycleAccumulatorsOnActivity(record);

			if (ShouldStartClosing(record))
			{
				record.stage = WorldStage::Closing;
				record.closingElapsedSec = 0.0;
			}
			break;
		}
		case WorldStage::Closing:
		{
			UpdateClosingElapsed(record, dtSec);
			UpdateEmptyElapsed(record, dtSec);

			if (ShouldEnterDestroyPending(record))
			{
				record.stage = WorldStage::DestroyPending;
			}
			break;
		}
		case WorldStage::DestroyPending:
		case WorldStage::Destroyed:
		case WorldStage::Faulted:
		default:
		{
			break;
		}
		}
	}
}

void WorldManager::CollectDestroyable()
{
	std::vector<WorldId> destroyList;
	destroyList.reserve(_records.size());

	for (const auto& [worldId, record] : _records)
	{
		if (record.stage == WorldStage::DestroyPending)
		{
			destroyList.push_back(worldId);
		}
	}

	for (WorldId worldId : destroyList)
	{
		auto it = _records.find(worldId);
		if (it == _records.end())
			continue;

		const WorldInstanceRecord& record = it->second;
		const WorldResolveKey key = MakeRecordResolveKey(record);

		if (_registry.DestroyWorld(worldId))
		{
			_resolveIndex.erase(key);
			_records.erase(it);
		}
	}
}

bool WorldManager::AddReservedSlots(WorldId worldId, uint32_t slots)
{
	WorldInstanceRecord* record = FindRecord(worldId);
	if (record == nullptr)
		return false;

	record->reservedSlots += slots;
	return true;
}

bool WorldManager::RemoveReservedSlots(WorldId worldId, uint32_t slots)
{
	WorldInstanceRecord* record = FindRecord(worldId);
	if (record == nullptr)
		return false;

	if (record->reservedSlots < slots)
		return false;

	record->reservedSlots -= slots;
	return true;
}

bool WorldManager::AddInflightTransferIn(WorldId worldId, uint32_t slots)
{
	WorldInstanceRecord* record = FindRecord(worldId);
	if (record == nullptr)
		return false;

	record->inFlightTransfersIn += slots;
	return true;
}

bool WorldManager::RemoveInflightTransferIn(WorldId worldId, uint32_t slots)
{
	WorldInstanceRecord* record = FindRecord(worldId);
	if (record == nullptr)
		return false;

	if (record->inFlightTransfersIn < slots)
		return false;

	record->inFlightTransfersIn -= slots;
	return true;
}

bool WorldManager::AddInflightTransferOut(WorldId worldId, uint32_t slots)
{
	WorldInstanceRecord* record = FindRecord(worldId);
	if (record == nullptr)
		return false;

	record->inFlightTransfersOut += slots;
	return true;
}

bool WorldManager::RemoveInflightTransferOut(WorldId worldId, uint32_t slots)
{
	WorldInstanceRecord* record = FindRecord(worldId);
	if (record == nullptr)
		return false;

	if (record->inFlightTransfersOut < slots)
		return false;

	record->inFlightTransfersOut -= slots;
	return true;
}

void WorldManager::FillRecordFromDef(
	WorldInstanceRecord& record,
	const WorldDef& def,
	uint64_t normalizedInstanceKey)
{
	record.defId = def.id;
	record.instanceType = def.topology.instanceType;
	record.instanceKey = normalizedInstanceKey;
	record.stage = WorldStage::Allocated;

	record.activePlayers = 0;
	record.reservedSlots = 0;
	record.inFlightTransfersIn = 0;
	record.inFlightTransfersOut = 0;

	record.joinPolicy = def.entryPolicy.joinPolicy;
	record.maxPlayerCount = def.entryPolicy.maxPlayerCount;
	record.allowReEntry = def.entryPolicy.allowReEntry;
	record.destroyWhenEmpty = def.entryPolicy.destroyWhenEmpty;
	record.emptyDestroyDelaySec =
		def.entryPolicy.emptyDestroyDelaySec.has_value()
		? static_cast<double>(*def.entryPolicy.emptyDestroyDelaySec)
		: 0.0;

	record.emptyElapsedSec = 0.0;
	record.closingElapsedSec = 0.0;
	record.closeRequested = false;
	record.completionReached = false;
}

bool WorldManager::TryFindAliveResolvedWorld(
	const WorldResolveKey& key,
	WorldId& outWorldId)
{
	auto it = _resolveIndex.find(key);
	if (it == _resolveIndex.end())
		return false;

	const WorldId existingId = it->second;
	if (_registry.IsAlive(existingId))
	{
		outWorldId = existingId;
		return true;
	}

	_resolveIndex.erase(it);
	return false;
}

WorldId WorldManager::CreateAndRegisterWorld(
	const WorldDef& def,
	uint64_t normalizedInstanceKey,
	const WorldResolveKey& key)
{
	WorldInstance* instance = _registry.CreateWorld(def, normalizedInstanceKey);
	if (instance == nullptr)
		return WorldId::Invalid();

	WorldInstanceRecord record;
	record.id = instance->GetId();
	FillRecordFromDef(record, def, normalizedInstanceKey);

	_records[record.id] = record;
	_resolveIndex[key] = record.id;

	return record.id;
}

WorldResolveKey WorldManager::MakeRecordResolveKey(const WorldInstanceRecord& record)
{
	WorldResolveKey key;
	key.defId = record.defId;
	key.instanceKey = record.instanceKey;
	return key;
}

bool WorldManager::HasClosingIntent(const WorldInstanceRecord& record)
{
	return record.closeRequested || record.completionReached;
}

bool WorldManager::IsDestroySafe(const WorldInstanceRecord& record)
{
	return record.HasNoOccupantsOrPendingWork();
}

bool WorldManager::ShouldStartClosing(const WorldInstanceRecord& record)
{
	if (HasClosingIntent(record))
		return true;

	// destroyWhenEmpty 자동 close는 persistent에는 적용하지 않음
	if (!record.destroyWhenEmpty)
		return false;

	if (record.instanceType == WorldInstanceType::Persistent)
		return false;

	return IsDestroySafe(record);
}

bool WorldManager::ShouldEnterDestroyPending(const WorldInstanceRecord& record)
{
	if (!HasClosingIntent(record) && !record.destroyWhenEmpty)
		return false;

	if (!IsDestroySafe(record))
		return false;

	if (record.emptyDestroyDelaySec <= 0.0)
		return true;

	return record.emptyElapsedSec >= record.emptyDestroyDelaySec;
}

void WorldManager::UpdateEmptyElapsed(WorldInstanceRecord& record, const double dtSec)
{
	if (!IsDestroySafe(record))
	{
		record.emptyElapsedSec = 0.0;
		return;
	}

	record.emptyElapsedSec += dtSec;
}

void WorldManager::UpdateClosingElapsed(WorldInstanceRecord& record, const double dtSec)
{
	record.closingElapsedSec += dtSec;
}

void WorldManager::ResetLifecycleAccumulatorsOnActivity(WorldInstanceRecord& record)
{
	if (!IsDestroySafe(record))
	{
		record.emptyElapsedSec = 0.0;
	}
}