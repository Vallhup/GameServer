#pragma once

#include <vector>

#include "WorldIds.h"
#include "AdmissionTypes.h"
#include "ITransferContext.h"
#include "WorldTransferRequest.h"

struct WorldTransferTxn
{
	TransferId id{ 0 };
	TransferStage stage{ TransferStage::Requested };

	std::vector<uint32_t> sessionIds;
	WorldId sourceWorldId{ WorldId::Invalid() };
	WorldTargetSpec target;

	PartyId partyId;

	bool allowFallback{ false };

	WorldId resolvedTargetWorldId{ WorldId::Invalid() };
	AdmissionReservation reservation;

	std::unique_ptr<ITransferContext> context;
	TransferFailureReason failReason{ TransferFailureReason::None };

	uint32_t retryCount{ 0 };
	bool rollbackRequired{ false };

	// [ side-effect tracking ]
	bool reservationConsumed{ false };
	bool targetInflightAdded{ false };
	bool sourceInflightAdded{ false };
	bool targetActivePlayersAdded{ false };
	bool sourceActivePlayersRemoved{ false };

	std::vector<uint32_t> importedSessionIds;
	std::vector<uint32_t> releasedSessionIds;

	double createdAtSec{ 0.0 };
	double updatedAtSec{ 0.0 };
	double deadlineSec{ 0.0 };

	inline uint32_t PlayerCount() const
	{
		return static_cast<uint32_t>(sessionIds.size());
	}

	inline uint32_t ContextPlayerCount() const
	{
		return context ? context->PlayerCount() : 0u;
	}

	inline uint32_t ImportedPlayerCount() const
	{
		return static_cast<uint32_t>(importedSessionIds.size());
	}

	inline uint32_t ReleasedPlayerCount() const
	{
		return static_cast<uint32_t>(releasedSessionIds.size());
	}

	inline bool HasResolvedTarget() const
	{
		return resolvedTargetWorldId.IsValid();
	}

	inline bool HasActiveReservation() const
	{
		return reservation.IsActiveReservation();
	}

	inline bool HasContext() const
	{
		return static_cast<bool>(context);
	}

	inline bool IsTerminal() const
	{
		return IsTransferTerminal(stage);
	}
};