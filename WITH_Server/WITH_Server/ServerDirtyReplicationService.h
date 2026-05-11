#pragma once

#include "FrameworkRuntime.h"
#include "NetworkRuntime.h"
#include "SessionFlowController.h"

#include <span>

class ServerDirtyReplicationService final {
public:
	static void BuildAndStage(
		FrameworkRuntime& framework,
		NetworkRuntime& network,
		SessionFlowController& sessionFlow,
		std::span<const SessionId> excludedSessionIds = {});
};
