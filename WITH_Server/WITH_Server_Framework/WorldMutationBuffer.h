#pragma once

#include <mutex>
#include <vector>

#include "WorldMutationCommand.h"

class WorldMutationBuffer final {
public:
	WorldMutationBuffer() = default;
	~WorldMutationBuffer() = default;

	WorldMutationBuffer(const WorldMutationBuffer&) = delete;
	WorldMutationBuffer& operator=(const WorldMutationBuffer&) = delete;

	template<WorldMutationCommandCallable T>
	void Enqueue(T&& fn)
	{
		Enqueue(WorldMutationCommand(std::forward<T>(fn)));
	}

	void Enqueue(WorldMutationCommand command);

	void Commit(WorldRuntime& rt);
	void Clear();
	bool Empty() const;

private:
	mutable std::mutex _mtx;
	std::vector<WorldMutationCommand> _pendingCommands;
};
