#pragma once

#include <cstdint>

struct InboundCommandStatsSnapshot
{
	uint64_t missingSessionBindingCount{ 0 };
	uint64_t netBindingNotFoundCount{ 0 };
	uint64_t worldNotFoundCount{ 0 };
	uint64_t enqueueFailedCount{ 0 };
	uint64_t moveCommandOverwrittenCount{ 0 };
	uint64_t actionCommandSuppressedCount{ 0 };
	uint64_t guardCommandOverwrittenCount{ 0 };
};

class InboundCommandStatsCollector final {
public:
	void Clear() noexcept;

	const InboundCommandStatsSnapshot& GetSnapshot() const noexcept
	{
		return _snapshot;
	}

	void AddMissingSessionBinding() noexcept;
	void AddNetBindingNotFound() noexcept;
	void AddWorldNotFound() noexcept;
	void AddEnqueueFailed() noexcept;
	void AddMoveCommandOverwritten() noexcept;
	void AddActionCommandSuppressed() noexcept;
	void AddGuardCommandOverwritten() noexcept;

private:
	InboundCommandStatsSnapshot _snapshot;
};
