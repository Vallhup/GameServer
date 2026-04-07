#pragma once

#include <mutex>
#include <vector>

#include "WorldLifecycleCommand.h"

class WorldLifecycleBuffer final {
public:
	void Enqueue(const WorldLifecycleCommand& command);
	void Enqueue(WorldLifecycleCommand&& command);

	void DrainTo(std::vector<WorldLifecycleCommand>& out);
	void Clear();
	bool Empty() const;

private:
	mutable std::mutex _mtx;
	std::vector<WorldLifecycleCommand> _pending;
};
