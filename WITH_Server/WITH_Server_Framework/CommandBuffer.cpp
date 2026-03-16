#include "pch.h"
#include "CommandBuffer.h"

void CommandBuffer::Commit(WorldRuntime& rt)
{
	std::vector<std::unique_ptr<ICommand>> local;
	{
		std::lock_guard lock{ _mtx };

		if (_pendingCommands.empty())
			return;

		local.swap(_pendingCommands);
	}

	for (auto& cmd : local)
	{
		cmd->Apply(rt);
	}
}

void CommandBuffer::Clear()
{
	std::lock_guard lock{ _mtx };
	_pendingCommands.clear();
}

bool CommandBuffer::Empty() const
{
	std::lock_guard lock{ _mtx };
	return _pendingCommands.empty();
}
