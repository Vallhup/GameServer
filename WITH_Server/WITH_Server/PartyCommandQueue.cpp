#include "pch.h"
#include "PartyCommandQueue.h"

void PartyCommandQueue::Submit(PartyCommand command)
{
	std::scoped_lock lock(_mutex);
	_commands.push_back(std::move(command));
}

void PartyCommandQueue::DrainInto(std::vector<PartyCommand>& out)
{
	std::deque<PartyCommand> drained;
	{
		std::scoped_lock lock(_mutex);
		drained.swap(_commands);
	}

	out.reserve(out.size() + drained.size());
	while (!drained.empty())
	{
		out.push_back(std::move(drained.front()));
		drained.pop_front();
	}
}

void PartyCommandQueue::Clear()
{
	std::scoped_lock lock(_mutex);
	_commands.clear();
}

bool PartyCommandQueue::Empty() const
{
	std::scoped_lock lock(_mutex);
	return _commands.empty();
}
