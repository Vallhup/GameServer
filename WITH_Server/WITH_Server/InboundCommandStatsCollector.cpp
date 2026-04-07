#include "pch.h"
#include "InboundCommandStatsCollector.h"

void InboundCommandStatsCollector::Clear() noexcept
{
	_snapshot = InboundCommandStatsSnapshot{};
}

void InboundCommandStatsCollector::AddMissingSessionBinding() noexcept
{
	++_snapshot.missingSessionBindingCount;
}

void InboundCommandStatsCollector::AddNetBindingNotFound() noexcept
{
	++_snapshot.netBindingNotFoundCount;
}

void InboundCommandStatsCollector::AddWorldNotFound() noexcept
{
	++_snapshot.worldNotFoundCount;
}

void InboundCommandStatsCollector::AddEnqueueFailed() noexcept
{
	++_snapshot.enqueueFailedCount;
}

void InboundCommandStatsCollector::AddMoveCommandOverwritten() noexcept
{
	++_snapshot.moveCommandOverwrittenCount;
}

void InboundCommandStatsCollector::AddActionCommandSuppressed() noexcept
{
	++_snapshot.actionCommandSuppressedCount;
}

void InboundCommandStatsCollector::AddGuardCommandOverwritten() noexcept
{
	++_snapshot.guardCommandOverwrittenCount;
}
