#include "pch.h"
#include "WorldTransferService.h"

#include "WorldManager.h"
#include "WorldInstanceRecord.h"

#include "PresenceManager.h"

#include "WorldAdmissionService.h"

#include "WorldTransferTxn.h"
#include "WorldTransferRequest.h"

WorldTransferService::WorldTransferService(
	WorldManager& worldManager, 
	WorldAdmissionService& admissionService, 
	PresenceManager& presenceManager)
	: _worldManager(worldManager)
	, _admissionService(admissionService)
	, _presenceManager(presenceManager)
{
}

TransferId WorldTransferService::EnqueueRequest(
	const WorldTransferRequest& request,
	double nowSec)
{
	if (!request.IsValid())
		return 0;

	WorldTransferTxn txn;
	txn.id = AllocateTransferId();
	txn.stage = TransferStage::Requested;

	txn.connectionIds = request.connectionIds;
	txn.sourceWorldId = request.sourceWorldId;
	txn.target = request.target;
	txn.partyId = request.partyId;

	txn.createdAtSec = nowSec;
	txn.updatedAtSec = nowSec;
	txn.deadlineSec = nowSec + 10.0;

	_txns.emplace(txn.id, std::move(txn));
	return txn.id;
}

void WorldTransferService::Tick(const double nowSec)
{
	for (auto& [id, txn] : _txns)
	{
		(void)id;

		if (txn.IsTerminal())
			continue;

		txn.updatedAtSec = nowSec;
		ProgressTxn(txn, nowSec);
	}
}

WorldTransferTxn* WorldTransferService::FindTxn(TransferId id)
{
	auto it = _txns.find(id);
	if (it == _txns.end())
		return nullptr;

	return &it->second;
}

const WorldTransferTxn* WorldTransferService::FindTxn(TransferId id) const
{
	auto it = _txns.find(id);
	if (it == _txns.end())
		return nullptr;

	return &it->second;
}

void WorldTransferService::ProgressTxn(WorldTransferTxn& txn, const double nowSec)
{
	switch (txn.stage) {
	case TransferStage::Requested:
	{
		if (StepValidateSource(txn))
			txn.stage = TransferStage::SourceValidated;
		else
			FailTxn(txn, TransferFailureReason::StaleSource, nowSec);
		break;
	}
	case TransferStage::SourceValidated:
	{
		if (StepResolveTarget(txn))
			txn.stage = TransferStage::TargetResolved;
		else
			FailTxn(txn, TransferFailureReason::TargetResolveFailed, nowSec);
		break;
	}
	case TransferStage::TargetResolved:
	{
		if (StepReserveAdmission(txn, nowSec))
			txn.stage = TransferStage::AdmissionReserved;
		else
			FailTxn(txn, TransferFailureReason::AdmissionRejected, nowSec);
		break;
	}
	case TransferStage::AdmissionReserved:
	{
		if (StepBuildSnapshots(txn))
			txn.stage = TransferStage::SnapshotBuilt;
		else
			FailTxn(txn, TransferFailureReason::SnapshotFailed, nowSec);
		break;
	}
	case TransferStage::SnapshotBuilt:
	{
		if (StepImportTarget(txn, nowSec))
			txn.stage = TransferStage::TargetImported;
		else
			FailTxn(txn, TransferFailureReason::ImportFailed, nowSec);
		break;
	}
	case TransferStage::TargetImported:
	{
		if (StepReleaseSource(txn, nowSec))
			txn.stage = TransferStage::SourceReleased;
		else
			FailTxn(txn, TransferFailureReason::SourceReleaseFailed, nowSec);
		break;
	}
	case TransferStage::SourceReleased:
	{
		CompleteTxn(txn);
		break;
	}
	case TransferStage::Completed:
	case TransferStage::Failed:
	default:
	{
		break;
	}
	}
}

bool WorldTransferService::StepValidateSource(WorldTransferTxn& txn)
{
	const WorldInstanceRecord* source = _worldManager.FindRecord(txn.sourceWorldId);
	if (source == nullptr)
		return false;

	if (!source->IsRunnable())
		return false;

	for (uint32_t connectionId : txn.connectionIds)
	{
		const PresenceRecord* presence = _presenceManager.FindByConnectionId(connectionId);
		if (presence == nullptr)
			return false;

		if (!presence->IsActiveInWorld(txn.sourceWorldId))
			return false;

		if (presence->IsTransfering())
			return false;
	}

	return true;
}

bool WorldTransferService::StepResolveTarget(WorldTransferTxn& txn)
{
	if (txn.target.explicitTargetId.has_value())
	{
		txn.resolvedTargetWorldId = *txn.target.explicitTargetId;
	}
	else
	{
		if (!txn.target.targetWorldDefId.has_value())
			return false;

		txn.resolvedTargetWorldId =
			_worldManager.ResolveOrCreate(*txn.target.targetWorldDefId, txn.target.instanceKey);
	}

	if (!txn.resolvedTargetWorldId.IsValid())
		return false;

	for (uint32_t connectionId : txn.connectionIds)
	{
		if (!_presenceManager.BeginTransfer(
			connectionId,
			txn.id,
			txn.sourceWorldId,
			txn.resolvedTargetWorldId,
			txn.updatedAtSec))
		{
			return false;
		}
	}

	return true;
}

bool WorldTransferService::StepReserveAdmission(WorldTransferTxn& txn, const double nowSec)
{
	AdmissionRequest request;
	request.targetWorldId = txn.resolvedTargetWorldId;
	request.connectionIds = txn.connectionIds;
	request.reason = AdmissionReason::Transfer;
	request.partyId = txn.partyId;

	const AdmissionResult result = _admissionService.TryReserve(request, nowSec);
	if (!result.IsAccepted())
		return false;

	txn.reservation = result.reservation;
	return true;
}

bool WorldTransferService::StepBuildSnapshots(WorldTransferTxn& txn)
{
	txn.snapshots.clear();
	txn.snapshots.reserve(txn.connectionIds.size());

	for (uint32_t connectionId : txn.connectionIds)
	{
		PlayerSnapshot snapshot;
		snapshot.connectionId = connectionId;
		txn.snapshots.push_back(snapshot);
	}

	return !txn.snapshots.empty();
}

bool WorldTransferService::StepImportTarget(WorldTransferTxn& txn, const double nowSec)
{
	WorldInstanceRecord* target = _worldManager.FindRecord(txn.resolvedTargetWorldId);
	if (target == nullptr)
		return false;

	// 角力 import/spawn篮 酒流 mock 贸府
	if (!_admissionService.ConsumeReservation(txn.reservation.ticket, nowSec))
		return false;

	if (!_worldManager.AddInflightTransferIn(target->id, txn.PlayerCount()))
		return false;

	for (uint32_t connectionId : txn.connectionIds)
	{
		if (!_presenceManager.MarkTargetImported(
			connectionId,
			txn.id,
			txn.resolvedTargetWorldId,
			nowSec))
		{
			return false;
		}
	}

	if (!_worldManager.RemoveInflightTransferIn(target->id, txn.PlayerCount()))
		return false;

	target->activePlayers += txn.PlayerCount();
	return true;
}

bool WorldTransferService::StepReleaseSource(WorldTransferTxn& txn, const double nowSec)
{
	WorldInstanceRecord* source = _worldManager.FindRecord(txn.sourceWorldId);
	if (source == nullptr)
		return false;

	if (!_worldManager.AddInflightTransferOut(source->id, txn.PlayerCount()))
		return false;

	if (source->activePlayers < txn.PlayerCount())
		return false;

	source->activePlayers -= txn.PlayerCount();

	if (!_worldManager.RemoveInflightTransferOut(source->id, txn.PlayerCount()))
		return false;

	for (uint32_t connectionId : txn.connectionIds)
	{
		if (!_presenceManager.CompleteTransfer(
			connectionId,
			txn.id,
			txn.sourceWorldId,
			txn.resolvedTargetWorldId,
			nowSec))
		{
			return false;
		}
	}

	return true;
}

void WorldTransferService::FailTxn(
	WorldTransferTxn& txn,
	TransferFailureReason reason,
	double nowSec)
{
	txn.failReason = reason;

	if (txn.reservation.IsActiveReservation())
	{
		_admissionService.CancelReservation(txn.reservation.ticket);
	}

	for (uint32_t connectionId : txn.connectionIds)
	{
		_presenceManager.FailTransfer(connectionId, txn.id, nowSec);
	}

	txn.stage = TransferStage::Failed;
}

void WorldTransferService::CompleteTxn(WorldTransferTxn& txn)
{
	txn.stage = TransferStage::Completed;
}

TransferId WorldTransferService::AllocateTransferId()
{
	return _nextTransferId++;
}