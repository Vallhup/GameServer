#include "pch.h"
#include "WorldCommandQueue.h"

bool WorldCommandQueue::Enqueue(WorldCommand command)
{
	std::lock_guard<std::mutex> lock(_mtx);

	command.enqueueOrder = _nextEnqueueOrder++;
	_pendingCommands.push_back(std::move(command));
	return true;
}

void WorldCommandQueue::DrainTo(
	std::vector<WorldCommand>& outFrameCommands)
{
	outFrameCommands.clear();

	std::lock_guard<std::mutex> lock(_mtx);
	_pendingCommands.swap(outFrameCommands);
}

void WorldCommandQueue::Clear()
{
	std::lock_guard<std::mutex> lock(_mtx);
	_pendingCommands.clear();
	_nextEnqueueOrder = 1;
}

size_t WorldCommandQueue::GetPendingCommandCount() const
{
	std::lock_guard<std::mutex> lock(_mtx);
	return _pendingCommands.size();
}
