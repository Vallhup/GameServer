#include "pch.h"
#include "WorldManager.h"

WorldManager::WorldManager(WorldRegistry& registry)
	: _registry(registry)
{
}

WorldId WorldManager::ResolveOrCreate(WorldDefId worldDefId, uint64_t instanceKey)
{
	// 1. def 조회
	// 2. validate
	// 3. resolve key 생성
	// 4. _resolveIndex 조회
	// 5. 있으면 기존 WorldId 반환
	// 6. 없으면 CreationPolicy 확인
	// 7. registry로 생성
	// 8. _records / _resolveIndex 등록
	// 9. 새 WorldId 반환
	(void)worldDefId;
	(void)instanceKey;
	return WorldId::Invalid();
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
