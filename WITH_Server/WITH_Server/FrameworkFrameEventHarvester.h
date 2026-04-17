#pragma once

#include "FrameworkRuntime.h"

class NetIdRegistry;
class WorldManager;
class WorldRegistry;

class FrameworkFrameEventHarvester final {
public:
	static void Harvest(
		WorldManager& worldManager,
		WorldRegistry& worldRegistry,
		NetIdRegistry& netIdRegistry,
		FrameworkRuntime::FrameResult::FrameEvents& outEvents);
};
