#pragma once

#include "WorldIds.h"
#include "WorldLifecycleEnums.h"

class WorldManager;
class WorldAdmissionService;

class PresenceManager;

struct WorldTransferTxn;
struct WorldTransferRequest;

class WorldTransferService {
public:
	WorldTransferService(
		WorldManager& worldManager,
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
	bool StepBuildSnapshots(WorldTransferTxn& txn);
	bool StepImportTarget(WorldTransferTxn& txn, const double nowSec);
	bool StepReleaseSource(WorldTransferTxn& txn, const double nowSec);

	void FailTxn(
		WorldTransferTxn& txn, 
		TransferFailureReason reason,
		const double nowSec
	);

	void CompleteTxn(WorldTransferTxn& txn);

	TransferId AllocateTransferId();

private:
	WorldManager& _worldManager;
	WorldAdmissionService& _admissionService;
	PresenceManager& _presenceManager;

	std::unordered_map<TransferId, WorldTransferTxn> _txns;
	TransferId _nextTransferId{ 1 };
};
