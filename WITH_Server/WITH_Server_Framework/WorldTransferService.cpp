#include "pch.h"
#include "WorldTransferService.h"

#include "WorldRegistry.h"

#include "WorldManager.h"
#include "WorldInstanceRecord.h"

#include "PresenceManager.h"

#include "IWorldTransferRuntimeBridge.h"

#include "WorldAdmissionService.h"

#include "WorldTransferTxn.h"
#include "WorldTransferRequest.h"

WorldTransferService::WorldTransferService(
	WorldManager& worldManager, 
	WorldAdmissionService& admissionService, 
	PresenceManager& presenceManager, 
	IWorldTransferRuntimeBridge& runtimeBridge)
	: _worldManager(worldManager)
	, _admissionService(admissionService)
	, _presenceManager(presenceManager)
	, _runtimeBridge(runtimeBridge)
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
		if (StepBuildTransferContext(txn))
			txn.stage = TransferStage::TransferContextBuilt;
		else
			FailTxn(txn, TransferFailureReason::TransferContextBuildFailed, nowSec);
		break;
	}
	case TransferStage::TransferContextBuilt:
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
	txn.reservationConsumed = false;
	return true;
}

bool WorldTransferService::StepBuildTransferContext(WorldTransferTxn& txn)
{
	txn.context.reset();

	if (!_runtimeBridge.BuildTransferContext(
		txn.sourceWorldId,
		txn.connectionIds,
		txn.context))
	{
		return false;
	}

	if (!txn.context)
		return false;

	if (txn.context->PlayerCount() == 0)
		return false;

	if (txn.context->PlayerCount() != txn.PlayerCount())
		return false;

	// TODO : 더 엄격하게 검사한다면
	//        txn.connectionIds == context 내부 connection 집합 일치 확인

	return true;
}

bool WorldTransferService::StepImportTarget(WorldTransferTxn& txn, const double nowSec)
{
	WorldInstanceRecord* target = _worldManager.FindRecord(txn.resolvedTargetWorldId);
	if (target == nullptr)
		return false;

	if (!txn.context)
		return false;

	txn.importedConnectionIds.clear();

	if (!_admissionService.ConsumeReservation(txn.reservation.ticket, nowSec))
		return false;

	txn.reservationConsumed = true;

	if (!_worldManager.AddInflightTransferIn(target->id, txn.PlayerCount()))
		return false;

	txn.targetInflightAdded = true;

	if (!_runtimeBridge.ImportTransferContext(
		txn.resolvedTargetWorldId,
		*txn.context,
		txn.importedConnectionIds))
	{
		return false;
	}

	if (txn.importedConnectionIds.empty())
		return false;

	for (uint32_t connectionId : txn.importedConnectionIds)
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

	txn.targetInflightAdded = false;

	target->activePlayers += txn.ImportedPlayerCount();
	txn.targetActivePlayersAdded = true;

	return txn.ImportedPlayerCount() == txn.PlayerCount();
}

bool WorldTransferService::StepReleaseSource(WorldTransferTxn& txn, const double nowSec)
{
	WorldInstanceRecord* source = _worldManager.FindRecord(txn.sourceWorldId);
	if (source == nullptr)
		return false;

	if (!txn.context)
		return false;

	txn.releasedConnectionIds.clear();

	if (!_worldManager.AddInflightTransferOut(source->id, txn.PlayerCount()))
		return false;

	txn.sourceInflightAdded = true;

	if (!_runtimeBridge.ReleaseTransferContext(
		txn.sourceWorldId,
		*txn.context,
		txn.releasedConnectionIds))
	{
		return false;
	}

	if (txn.releasedConnectionIds.empty())
		return false;

	for (uint32_t connectionId : txn.releasedConnectionIds)
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

	if (source->activePlayers < txn.ReleasedPlayerCount())
		return false;

	source->activePlayers -= txn.ReleasedPlayerCount();
	txn.sourceActivePlayersRemoved = true;

	if (!_worldManager.RemoveInflightTransferOut(source->id, txn.PlayerCount()))
		return false;

	txn.sourceInflightAdded = false;

	return txn.ReleasedPlayerCount() == txn.PlayerCount();
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
	case TransferStage::TransferContextBuilt:
	{
		CleanupReservationOnFailure(txn);
		CleanupPresenceOnFailure(txn, nowSec);
		txn.context.reset();
		break;
	}
	case TransferStage::TargetImported:
	{
		CleanupReservationOnFailure(txn);
		CleanupImportedTargetOnFailure(txn);
		CleanupPresenceOnFailure(txn, nowSec);
		txn.context.reset();
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

	CleanupSourceInflightOnFailure(txn);
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

	if (!txn.importedConnectionIds.empty())
	{
		_runtimeBridge.RollbackImportedTransferContext(
			txn.resolvedTargetWorldId,
			*txn.context,
			txn.importedConnectionIds);
	}

	if (txn.targetActivePlayersAdded)
	{
		const uint32_t importedCount = txn.ImportedPlayerCount();

		if (target->activePlayers >= importedCount)
		{
			target->activePlayers -= importedCount;
		}
		else
		{
			target->activePlayers = 0;
		}

		txn.targetActivePlayersAdded = false;
	}

	if (txn.targetInflightAdded)
	{
		_worldManager.RemoveInflightTransferIn(
			txn.resolvedTargetWorldId,
			txn.PlayerCount());
		txn.targetInflightAdded = false;
	}
}

void WorldTransferService::CleanupSourceInflightOnFailure(WorldTransferTxn& txn)
{
	if (!txn.sourceInflightAdded)
		return;

	_worldManager.RemoveInflightTransferOut(txn.sourceWorldId, txn.PlayerCount());
	txn.sourceInflightAdded = false;
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
	txn.context.reset();
	txn.failReason = TransferFailureReason::None;
	txn.rollbackRequired = false;

	txn.reservationConsumed = false;
	txn.targetInflightAdded = false;
	txn.sourceInflightAdded = false;
	txn.targetActivePlayersAdded = false;
	txn.sourceActivePlayersRemoved = false;
	txn.importedConnectionIds.clear();
	txn.releasedConnectionIds.clear();

	txn.updatedAtSec = nowSec;
	txn.deadlineSec = nowSec + 10.0;
	++txn.retryCount;

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