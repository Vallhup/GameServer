#pragma once

#include "FrameworkRuntime.h"
#include "NetworkRuntime.h"
#include "PlayerEntryService.h"
#include "SessionBindingRegistry.h"

class ServerFrameEventDispatcher final {
public:
	static bool Dispatch(
		const FrameworkRuntime::FrameResult& frameResult,
		FrameworkRuntime& framework,
		NetworkRuntime& network,
		SessionBindingRegistry& sessionBindings,
		PlayerEntryService& playerEntryService,
		double nowSec);
};
