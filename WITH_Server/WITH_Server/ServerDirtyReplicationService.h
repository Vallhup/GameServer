#pragma once

#include "FrameworkRuntime.h"
#include "NetworkRuntime.h"
#include "SessionBindingRegistry.h"

class ServerDirtyReplicationService final {
public:
	static void BuildAndStage(
		FrameworkRuntime& framework,
		NetworkRuntime& network,
		SessionBindingRegistry& sessionBindings);
};
