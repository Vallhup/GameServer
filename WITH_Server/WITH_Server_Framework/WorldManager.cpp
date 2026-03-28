#include "pch.h"
#include "WorldManager.h"

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

	// 1. def 조회
	const WorldDef* def = _registry.FindWorldDef(worldDefId);
	if (def == nullptr)
		return WorldId::Invalid();

	// 2. 입력 검증
	if (ValidateResolveInput(*def, instanceKey) != WorldResolveValidationResult::Success)
		return WorldId::Invalid();

	// 3. resolve key 생성
	const WorldResolveKey key = MakeResolveKey(*def, instanceKey);

	// 4. 기존 resolve index 조회
	{
		auto it = _resolveIndex.find(key);
		if (it != _resolveIndex.end())
		{
			const WorldId existingId = it->second;
			if (_registry.IsAlive(existingId))
				return existingId;

			_resolveIndex.erase(it);
		}
	}

	// 5. PreCreated인데 기존 인스턴스 없으면 실패
	if (def->entryPolicy.creationPolicy == CreationPolicy::PreCreated)
		return WorldId::Invalid();

	// 6. 새 world 생성
	WorldInstance* instance = _registry.CreateWorld(*def, key.instanceKey);
	if (instance == nullptr)
		return WorldId::Invalid();

	// 7. record 생성
	WorldInstanceRecord record;
	record.id = instance->GetId();
	FillRecordFromDef(record, *def, key.instanceKey);

	// 8. 등록
	_records[record.id] = record;
	_resolveIndex[key] = record.id;

	return record.id;
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

	{
		auto it = _resolveIndex.find(key);
		if (it != _resolveIndex.end())
		{
			const WorldId existingId = it->second;
			if (_registry.IsAlive(existingId))
				return existingId;

			_resolveIndex.erase(it);
		}
	}

	WorldInstance* instance = _registry.CreateWorld(*def, key.instanceKey);
	if (instance == nullptr)
		return WorldId::Invalid();

	WorldInstanceRecord record;
	record.id = instance->GetId();
	FillRecordFromDef(record, *def, key.instanceKey);

	_records[record.id] = record;
	_resolveIndex[key] = record.id;

	return record.id;
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

void WorldManager::FlushLifecycle()
{
}

void WorldManager::CollectDestroyable()
{
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

	record.joinPolicy = def.entryPolicy.joinPolicy;
	record.maxPlayerCount = def.entryPolicy.maxPlayerCount;
	record.allowReEntry = def.entryPolicy.allowReEntry;
	record.destroyWhenEmpty = def.entryPolicy.destroyWhenEmpty;
	record.emptyDestroyDelaySec =
		def.entryPolicy.emptyDestroyDelaySec.has_value()
		? static_cast<double>(*def.entryPolicy.emptyDestroyDelaySec)
		: 0.0;
}
