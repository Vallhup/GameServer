#pragma once

#include <concurrent_queue.h>
#include "WorldLeapData.h"

class WorldRegistry;
class WorldService;

class WorldLeapManager {
public:
	explicit WorldLeapManager(WorldRegistry& reg, WorldService& service)
		: _reg(reg), _service(service) {}

	void Request(const WorldLeapRequest& request);
	void CommitFrame();

private:

	void HandleDefault(const WorldLeapRequest& request);
	void HandleRecoverToSquare(const WorldLeapRequest& request);

	WorldRegistry& _reg;
	WorldService& _service;

	concurrency::concurrent_queue<WorldLeapRequest> _requests;
};

