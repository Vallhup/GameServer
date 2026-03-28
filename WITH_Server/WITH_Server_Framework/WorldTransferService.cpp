#include "pch.h"
#include "WorldTransferService.h"

#include "WorldRegistry.h"

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
	txn.allowFallback = request.allowFallback;

	txn.createdAtSec = nowSec;
	txn.updatedAtSec = nowSec;
	txn.deadlineSec = nowSec + 10.0;

	_txns.try_emplace(txn.id, std::move(txn));
	return txn.id;
}

void WorldTransferService::Tick(const double nowSec)
{
	for (auto& [_, txn] : _txns)
	{
		if (txn.IsTerminal())
			continue;

		txn.updatedAtSec = nowSec;

		if (IsTimedOut(txn, nowSec))
		{
			FailTxn(txn, TransferFailureReason::TimedOut, nowSec);
			continue;
		}

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

	// 실제 import/spawn은 아직 mock 처리
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

bool WorldTransferService::IsTimedOut(const WorldTransferTxn& txn, const double nowSec) const
{
	return
		txn.deadlineSec > 0.0 &&
		nowSec >= txn.deadlineSec;
}

void WorldTransferService::CleanupFailedTxn(WorldTransferTxn& txn, const double nowSec)
{
	switch (txn.stage) {
	case TransferStage::Requested:
	case TransferStage::SourceValidated:
	case TransferStage::TargetResolved:
	{
		CleanupPresenceOnFailure(txn, nowSec);
		break;
	}
	case TransferStage::AdmissionReserved:
	case TransferStage::SnapshotBuilt:
	{
		CleanupReservationOnFailure(txn);
		CleanupPresenceOnFailure(txn, nowSec);
		txn.snapshots.clear();
		break;
	}
	case TransferStage::TargetImported:
	{
		CleanupReservationOnFailure(txn);
		CleanupImportedTargetOnFailure(txn);
		CleanupPresenceOnFailure(txn, nowSec);
		txn.snapshots.clear();
		break;
	}
	case TransferStage::SourceReleased:
	case TransferStage::Completed:
	case TransferStage::Failed:
	default:
	{
		break;
	}
	}

}

void WorldTransferService::CleanupPresenceOnFailure(WorldTransferTxn& txn, const double nowSec)
{
	for (uint32_t connectionId : txn.connectionIds)
	{
		_presenceManager.FailTransfer(connectionId, txn.id, nowSec);
	}
}

void WorldTransferService::CleanupReservationOnFailure(WorldTransferTxn& txn)
{
	if (txn.reservation.IsActiveReservation())
	{
		_admissionService.CancelReservation(txn.reservation.ticket);
	}
}

void WorldTransferService::CleanupImportedTargetOnFailure(WorldTransferTxn& txn)
{
	if (!txn.resolvedTargetWorldId.IsValid())
		return;

	WorldInstanceRecord* target = _worldManager.FindRecord(txn.resolvedTargetWorldId);
	if (target == nullptr)
		return;

	if (target->activePlayers >= txn.PlayerCount())
	{
		target->activePlayers -= txn.PlayerCount();
	}
	else
	{
		target->activePlayers = 0;
	}
}

void WorldTransferService::FailTxn(
	WorldTransferTxn& txn,
	TransferFailureReason reason,
	double nowSec)
{
	txn.failReason = reason;

	if (reason == TransferFailureReason::TimedOut)
	{
		++txn.retryCount;
	}

	if (txn.stage == TransferStage::TargetImported ||
		reason == TransferFailureReason::SourceReleaseFailed)
	{
		txn.rollbackRequired = true;
	}

	CleanupFailedTxn(txn, nowSec);

	if (TryApplyFallback(txn, reason, nowSec))
	{
		return;
	}

	txn.stage = TransferStage::Failed;
}

void WorldTransferService::CompleteTxn(WorldTransferTxn& txn)
{
	txn.stage = TransferStage::Completed;
}

bool WorldTransferService::CanFallback(
	const WorldTransferTxn& txn,
	TransferFailureReason reason) const
{
	if (!txn.allowFallback)
		return false;

	switch (reason) {
	case TransferFailureReason::TargetResolveFailed:
	case TransferFailureReason::AdmissionRejected:
	{
		return true;
	}
	case TransferFailureReason::TimedOut:
	{
		return
			txn.stage == TransferStage::Requested ||
			txn.stage == TransferStage::SourceValidated ||
			txn.stage == TransferStage::TargetResolved ||
			txn.stage == TransferStage::AdmissionReserved;
	}
	default:
	{
		return false;
	}
	}
}

bool WorldTransferService::TryApplyFallback(
	WorldTransferTxn& txn,
	TransferFailureReason reason,
	const double nowSec)
{
	if (!CanFallback(txn, reason))
		return false;

	const std::optional<WorldTargetSpec> fallbackTarget = BuildFallbackTarget(txn);
	if (!fallbackTarget.has_value())
		return false;

	txn.target = *fallbackTarget;
	txn.resolvedTargetWorldId = WorldId::Invalid();
	txn.reservation = AdmissionReservation{};
	txn.snapshots.clear();
	txn.failReason = TransferFailureReason::None;
	txn.rollbackRequired = false;
	txn.updatedAtSec = nowSec;
	txn.deadlineSec = nowSec + 10.0;
	++txn.retryCount;

	// source 검증은 이미 끝난 것으로 보고 target resolve부터 재시도
	txn.stage = TransferStage::SourceValidated;
	return true;
}

std::optional<WorldTargetSpec> WorldTransferService::BuildFallbackTarget(
	const WorldTransferTxn& txn) const
{
	if (txn.target.explicitTargetId.has_value())
		return std::nullopt;

	if (!txn.target.targetWorldDefId.has_value())
		return std::nullopt;

	const WorldDef* targetDef =
		_worldManager.GetRegistry().FindWorldDef(*txn.target.targetWorldDefId);
	if (targetDef == nullptr)
		return std::nullopt;

	if (!targetDef->entryPolicy.fallbackWorldDefId.has_value())
		return std::nullopt;

	WorldTargetSpec spec;
	spec.targetWorldDefId = *targetDef->entryPolicy.fallbackWorldDefId;
	spec.instanceKey = 0;
	return spec;
}

TransferId WorldTransferService::AllocateTransferId()
{
	return _nextTransferId++;
}