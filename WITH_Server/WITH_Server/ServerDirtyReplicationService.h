#pragma once

#include "FrameworkRuntime.h"
#include "NetworkRuntime.h"
#include "SessionBindingRegistry.h"

#include <span>

class ServerDirtyReplicationService final {
public:
	static void BuildAndStage(
		FrameworkRuntime& framework,
		NetworkRuntime& network,
		SessionBindingRegistry& sessionBindings,
		std::span<const SessionId> excludedSessionIds = {});
};
