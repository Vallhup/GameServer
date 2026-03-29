#pragma once

#include <optional>

#include "WorldIds.h"
#include "WorldTargetSpec.h"
#include "WorldLifecycleEnums.h"

class WorldManager;
class WorldRegistry;
class WorldAdmissionService;

class PresenceManager;

class IWorldTransferRuntimeBridge;

struct WorldTransferTxn;
struct WorldTransferRequest;

class WorldTransferService {
public:
	WorldTransferService(
		WorldManager& worldManager,
		WorldRegistry& worldRegistry,
		WorldAdmissionService& admissionService,
		PresenceManager& presenceManager);

	TransferId EnqueueRequest(const WorldTransferRequest& request, const double nowSec);

	void Tick(const double nowSec);

	WorldTransferTxn* FindTxn(TransferId id);
	const WorldTransferTxn* FindTxn(TransferId id) const;

private:
	void ProgressTxn(WorldTransferTxn& txn, const double nowSec);

	bool StepValidateSource(WorldTransferTxn& txn);
	bool StepResolveTarget(WorldTransferTxn& txn);
	bool StepReserveAdmission(WorldTransferTxn& txn, const double nowSec);
	bool StepBuildTransferContext(WorldTransferTxn& txn);
	bool StepImportTarget(WorldTransferTxn& txn, const double nowSec);
	bool StepReleaseSource(WorldTransferTxn& txn, const double nowSec);

	bool IsTimedOut(const WorldTransferTxn& txn, const double nowSec) const;

	void CleanupFailedTxn(WorldTransferTxn& txn, const double nowSec);
	void CleanupPresenceOnFailure(WorldTransferTxn& txn, const double nowSec);
	void CleanupReservationOnFailure(WorldTransferTxn& txn);
	void CleanupImportedTargetOnFailure(WorldTransferTxn& txn);
	void CleanupSourceInflightOnFailure(WorldTransferTxn& txn);

	void FailTxn(
		WorldTransferTxn& txn, 
		TransferFailureReason reason,
		const double nowSec
	);
	void CompleteTxn(WorldTransferTxn& txn);

	bool CanFallback(const WorldTransferTxn& txn, TransferFailureReason reason) const;
	bool TryApplyFallback(WorldTransferTxn& txn, TransferFailureReason reason, const double nowSec);
	std::optional<WorldTargetSpec> BuildFallbackTarget(const WorldTransferTxn& txn) const;

	TransferId AllocateTransferId();

private:
	WorldManager& _worldManager;
	WorldRegistry& _worldRegistry;
	WorldAdmissionService& _admissionService;
	PresenceManager& _presenceManager;

	std::unordered_map<TransferId, WorldTransferTxn> _txns;
	TransferId _nextTransferId{ 1 };
};

// explicitTargetId가 들어온 요청은 fallback X
// 
// fallbackWorldDefId가 없는 월드는 fallback X
// 
// fallback instanceKey는 1차 구현에서 0으로 고정
// 
// TransferContextBuilt 이후 실페에는 fallback 적용 X