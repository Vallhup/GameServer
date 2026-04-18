#pragma once

#include "FrameworkRuntime.h"
#include "SessionBindingRegistry.h"
#include "WorldTransferEvents.h"

class ServerWorldTransferCommitter final {
public:
	static bool Commit(
		FrameworkRuntime& framework,
		SessionBindingRegistry& sessionBindings);

	static bool Commit(
		FrameworkRuntime& framework,
		SessionBindingRegistry& sessionBindings,
		const WorldTransferEventBatch& events);

private:
	static bool CommitCompleted(
		FrameworkRuntime& framework,
		SessionBindingRegistry& sessionBindings,
		const WorldTransferCompletedEvent& event);

	static void HandleFailed(const WorldTransferFailedEvent& event);

	static bool ValidateCompleted(
		const FrameworkRuntime& framework,
		const SessionBindingRegistry& sessionBindings,
		const WorldTransferCompletedEvent& event);
};
