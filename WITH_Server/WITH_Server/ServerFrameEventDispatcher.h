#pragma once

#include "FrameworkRuntime.h"
#include "ServerSessionSystem.h"

#include <span>

class ServerFrameEventDispatcher final {
public:
	static bool Dispatch(
		const FrameworkRuntime::FrameResult& frameResult,
		FrameworkRuntime& framework,
		ServerSessionSystem& sessionSystem,
		double nowSec,
		std::span<const SessionId> excludedSessionIds = {});
};
