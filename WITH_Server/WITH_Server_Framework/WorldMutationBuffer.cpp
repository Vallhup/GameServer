#include "pch.h"
#include "WorldMutationBuffer.h"
#include "WorldRuntime.h"

void WorldMutationBuffer::Enqueue(WorldMutationCommand command)
{
	if (!command.IsValid())
		return;

	std::lock_guard lock{ _mtx };
	_pendingCommands.push_back(std::move(command));
}

void WorldMutationBuffer::Commit(WorldRuntime& rt)
{
	std::vector<WorldMutationCommand> local;
	{
		std::lock_guard lock{ _mtx };

		if (_pendingCommands.empty())
			return;

		local.swap(_pendingCommands);
	}

	for (auto& command : local)
	{
		if (rt.IsFaulted())
			break;

		command.Apply(rt);

		if (rt.IsFaulted())
			break;
	}
}

void WorldMutationBuffer::Clear()
{
	std::lock_guard lock{ _mtx };
	_pendingCommands.clear();
}

bool WorldMutationBuffer::Empty() const
{
	std::lock_guard lock{ _mtx };
	return _pendingCommands.empty();
}
