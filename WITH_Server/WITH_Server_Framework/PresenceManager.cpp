#include "pch.h"
#include "PresenceManager.h"

PresenceRecord* PresenceManager::FindByConnectionId(uint32_t connectionId)
{
	auto it = _records.find(connectionId);
	if (it == _records.end())
		return nullptr;

	return &it->second;
}

const PresenceRecord* PresenceManager::FindByConnectionId(uint32_t connectionId) const
{
	auto it = _records.find(connectionId);
	if (it == _records.end())
		return nullptr;

	return &it->second;
}

PresenceRecord* PresenceManager::EnsurePresence(uint32_t connectionId)
{
	auto [it, inserted] = _records.try_emplace(connectionId);
	PresenceRecord& record = it->second;

	if (inserted)
	{
		record.id = _nextPresenceId++;
		record.connectionId = connectionId;
		record.state = PresenceStage::None;
	}

	return &record;
}

bool PresenceManager::AttachToWorld(
	uint32_t connectionId, 
	WorldId worldId, 
	const double nowSec)
{
	PresenceRecord* record = EnsurePresence(connectionId);
	if (record == nullptr)
		return false;

	record->state = PresenceStage::Active;
	record->currentWorldId = worldId;
	record->sourceWorldId = WorldId::Invalid();
	record->targetWorldId = WorldId::Invalid();
	record->activeTransferId = 0;
	record->updatedAtSec = nowSec;
	return true;
}

bool PresenceManager::BeginTransfer(
	uint32_t connectionId,
	TransferId transferId,
	WorldId sourceWorldId,
	WorldId targetWorldId,
	const double nowSec)
{
	PresenceRecord* record = FindByConnectionId(connectionId);
	if (record == nullptr)
		return false;

	if (!record->HasCurrentWorld())
		return false;

	if (record->currentWorldId != sourceWorldId)
		return false;

	if (record->IsTransfering())
		return false;

	record->state = PresenceStage::TransferPending;
	record->sourceWorldId = sourceWorldId;
	record->targetWorldId = targetWorldId;
	record->activeTransferId = transferId;
	record->updatedAtSec = nowSec;
	return true;
}

bool PresenceManager::MarkTargetImported(
	uint32_t connectionId,
	TransferId transferId,
	WorldId targetWorldId,
	const double nowSec)
{
	PresenceRecord* record = FindByConnectionId(connectionId);
	if (record == nullptr)
		return false;

	if (record->activeTransferId != transferId)
		return false;

	if (record->targetWorldId != targetWorldId)
		return false;

	record->state = PresenceStage::ImportedToTarget;
	record->updatedAtSec = nowSec;
	return true;
}

bool PresenceManager::CompleteTransfer(
	uint32_t connectionId,
	TransferId transferId,
	WorldId sourceWorldId,
	WorldId targetWorldId,
	const double nowSec)
{
	PresenceRecord* record = FindByConnectionId(connectionId);
	if (record == nullptr)
		return false;

	if (record->activeTransferId != transferId)
		return false;

	if (record->sourceWorldId != sourceWorldId)
		return false;

	if (record->targetWorldId != targetWorldId)
		return false;

	record->state = PresenceStage::Active;
	record->currentWorldId = targetWorldId;
	record->sourceWorldId = WorldId::Invalid();
	record->targetWorldId = WorldId::Invalid();
	record->activeTransferId = 0;
	record->updatedAtSec = nowSec;
	return true;
}

bool PresenceManager::FailTransfer(
	uint32_t connectionId,
	TransferId transferId,
	const double nowSec)
{
	PresenceRecord* record = FindByConnectionId(connectionId);
	if (record == nullptr)
		return false;

	if (record->activeTransferId != transferId)
		return false;

	record->state = PresenceStage::Active;;
	record->targetWorldId = WorldId::Invalid();
	record->activeTransferId = 0;
	record->updatedAtSec = nowSec;
	return true;
}

bool PresenceManager::RemovePresence(uint32_t connectionId, const double nowSec)
{
	PresenceRecord* record = FindByConnectionId(connectionId);
	if (record == nullptr)
		return false;

	record->state = PresenceStage::Removed;
	record->currentWorldId = WorldId::Invalid();
	record->sourceWorldId = WorldId::Invalid();
	record->targetWorldId = WorldId::Invalid();
	record->activeTransferId = 0;
	record->updatedAtSec = nowSec;
	return true;
}

bool PresenceManager::SetDisconnected(
	uint32_t connectionId, 
	bool disconnected,
	const double nowSec)
{
	PresenceRecord* record = EnsurePresence(connectionId);
	if (record == nullptr)
		return false;

	record->disconnected = disconnected;
	record->state = disconnected ? PresenceStage::Disconnected : record->state;
	record->updatedAtSec = nowSec;
	return true;
}