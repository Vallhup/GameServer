#pragma once

#include <vector>

#include "WorldIds.h"
#include "AdmissionTypes.h"
#include "WorldTransferRequest.h"

struct PlayerSnapshot
{
	uint32_t connectionId{ 0 };
};

struct WorldTransferTxn
{
	TransferId id{ 0 };
	TransferStage stage{ TransferStage::Requested };

	std::vector<uint32_t> connectionIds;
	WorldId sourceWorldId{ WorldId::Invalid() };
	WorldTargetSpec target;

	PartyId partyId;

	WorldId resolvedTargetWorldId{ WorldId::Invalid() };
	AdmissionReservation reservation;

	std::vector<PlayerSnapshot> snapshots;
	TransferFailureReason failReason{ TransferFailureReason::None };

	uint32_t retryCount{ 0 };
	bool rollbackRequired{ false };

	double createdAtSec{ 0.0 };
	double updatedAtSec{ 0.0 };
	double deadlineSec{ 0.0 };

	inline uint32_t PlayerCount() const
	{
		return static_cast<uint32_t>(connectionIds.size());
	}

	inline bool HasResolvedTarget() const
	{
		return resolvedTargetWorldId.IsValid();
	}

	inline bool HasActiveReservation() const
	{
		return reservation.IsActiveReservation();
	}

	inline bool HasSnapshots() const
	{
		return !snapshots.empty();
	}

	inline bool IsTerminal() const
	{
		return IsTransferTerminal(stage);
	}
};