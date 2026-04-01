#include "pch.h"
#include "PresenceManager.h"

PresenceRecord* PresenceManager::FindBySessionId(uint32_t sessionId)
{
	auto it = _records.find(sessionId);
	if (it == _records.end())
		return nullptr;

	return &it->second;
}

const PresenceRecord* PresenceManager::FindBySessionId(uint32_t sessionId) const
{
	auto it = _records.find(sessionId);
	if (it == _records.end())
		return nullptr;

	return &it->second;
}

PresenceRecord* PresenceManager::EnsurePresence(uint32_t sessionId)
{
	auto [it, inserted] = _records.try_emplace(sessionId);
	PresenceRecord& record = it->second;

	if (inserted)
	{
		record.id = _nextPresenceId++;
		record.sessionId = sessionId;
		record.state = PresenceStage::None;
	}

	return &record;
}

bool PresenceManager::AttachToWorld(
	uint32_t sessionId, 
	WorldId worldId, 
	const double nowSec)
{
	PresenceRecord* record = EnsurePresence(sessionId);
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
	uint32_t sessionId,
	TransferId transferId,
	WorldId sourceWorldId,
	WorldId targetWorldId,
	const double nowSec)
{
	PresenceRecord* record = FindBySessionId(sessionId);
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
	uint32_t sessionId,
	TransferId transferId,
	WorldId targetWorldId,
	const double nowSec)
{
	PresenceRecord* record = FindBySessionId(sessionId);
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
	uint32_t sessionId,
	TransferId transferId,
	WorldId sourceWorldId,
	WorldId targetWorldId,
	const double nowSec)
{
	PresenceRecord* record = FindBySessionId(sessionId);
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
	uint32_t sessionId,
	TransferId transferId,
	const double nowSec)
{
	PresenceRecord* record = FindBySessionId(sessionId);
	if (record == nullptr)
		return false;

	if (record->activeTransferId != transferId)
		return false;

	record->state = PresenceStage::Active;;
	record->sourceWorldId = WorldId::Invalid();
	record->targetWorldId = WorldId::Invalid();
	record->activeTransferId = 0;
	record->updatedAtSec = nowSec;
	return true;
}

bool PresenceManager::RemovePresence(uint32_t sessionId, const double nowSec)
{
	PresenceRecord* record = FindBySessionId(sessionId);
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
	uint32_t sessionId, 
	bool disconnected,
	const double nowSec)
{
	PresenceRecord* record = EnsurePresence(sessionId);
	if (record == nullptr)
		return false;

	record->disconnected = disconnected;
	record->state = disconnected ? PresenceStage::Disconnected : record->state;
	record->updatedAtSec = nowSec;
	return true;
}

bool PresenceManager::CanReEnterWorld(uint32_t sessionId, WorldId targetWorldId) const
{
	const PresenceRecord* record = FindBySessionId(sessionId);
	if (record == nullptr)
		return false;

	if (!targetWorldId.IsValid())
		return false;

	return record->CanReEnterWorld(targetWorldId);
}
