#pragma once

#include <cstdint>
#include <vector>

#include "WorldIds.h"
#include "WorldLifecycleEnums.h"
#include "WorldTargetSpec.h"
#include "WorldTransferTypes.h"

struct WorldTransferCompletedEvent
{
	TransferId transferId{ 0 };

	WorldId sourceWorldId{ WorldId::Invalid() };
	WorldId targetWorldId{ WorldId::Invalid() };
	WorldTargetSpec requestedTarget;

	PartyId partyId{ 0 };
	std::vector<uint32_t> sessionIds;

	std::vector<ImportedTransferEntity> importedEntities;
	std::vector<uint32_t> releasedSessionIds;

	uint32_t retryCount{ 0 };
	bool usedFallback{ false };

	double completedAtSec{ 0.0 };
};

struct WorldTransferFailedEvent
{
	TransferId transferId{ 0 };

	WorldId sourceWorldId{ WorldId::Invalid() };
	WorldId resolvedTargetWorldId{ WorldId::Invalid() };
	WorldTargetSpec requestedTarget;

	PartyId partyId{ 0 };
	std::vector<uint32_t> sessionIds;

	TransferStage failedStage{ TransferStage::Failed };
	TransferFailureReason reason{ TransferFailureReason::None };

	std::vector<ImportedTransferEntity> importedEntities;

	uint32_t retryCount{ 0 };
	bool rollbackRequired{ false };

	double failedAtSec{ 0.0 };
};

struct WorldTransferEventBatch
{
	std::vector<WorldTransferCompletedEvent> completed;
	std::vector<WorldTransferFailedEvent> failed;

	bool Empty() const noexcept
	{
		return completed.empty() && failed.empty();
	}

	void Clear() noexcept
	{
		completed.clear();
		failed.clear();
	}
};
