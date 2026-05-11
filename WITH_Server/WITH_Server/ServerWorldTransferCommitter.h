#pragma once

#include "FrameworkRuntime.h"
#include "SessionFlowController.h"
#include "WorldTransferEvents.h"

class ServerWorldTransferCommitter final {
public:
	static bool Commit(
		FrameworkRuntime& framework,
		SessionFlowController& sessionFlow);

	static bool Commit(
		FrameworkRuntime& framework,
		SessionFlowController& sessionFlow,
		const WorldTransferEventBatch& events);

private:
	static bool CommitCompleted(
		FrameworkRuntime& framework,
		SessionFlowController& sessionFlow,
		const WorldTransferCompletedEvent& event);

	static void HandleFailed(const WorldTransferFailedEvent& event);

	static bool ValidateCompleted(
		const FrameworkRuntime& framework,
		const SessionFlowController& sessionFlow,
		const WorldTransferCompletedEvent& event);
};
