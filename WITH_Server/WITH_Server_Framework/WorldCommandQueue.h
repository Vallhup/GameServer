#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

#include "WorldCommand.h"

class WorldCommandQueue final
{
public:
	WorldCommandQueue() = default;
	~WorldCommandQueue() = default;

	WorldCommandQueue(const WorldCommandQueue&) = delete;
	WorldCommandQueue& operator=(const WorldCommandQueue&) = delete;

	WorldCommandQueue(WorldCommandQueue&&) = delete;
	WorldCommandQueue& operator=(WorldCommandQueue&&) = delete;

	bool Enqueue(WorldCommand command);
	void DrainTo(std::vector<WorldCommand>& outFrameCommands);
	void Clear();

	size_t GetPendingCommandCount() const;

private:
	mutable std::mutex _mtx;
	std::vector<WorldCommand> _pendingCommands;
	uint64_t _nextEnqueueOrder{ 1 };
};
