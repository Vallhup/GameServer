#pragma once

#include <vector>

#include "AdmissionTypes.h"
#include "WorldTransferRequest.h"

using TransferId = uint64_t;

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

	WorldId resolvedTargetWorldId{ WorldId::Invalid() };
	AdmissionReservation reservation;

	std::vector<PlayerSnapshot> snapshots;
	TransferFailureReason failReason{ TransferFailureReason::None };
};