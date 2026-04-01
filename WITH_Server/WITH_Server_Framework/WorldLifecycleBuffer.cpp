#include "pch.h"
#include "WorldLifecycleBuffer.h"

#include <algorithm>

void WorldLifecycleBuffer::Enqueue(const WorldLifecycleCommand& command)
{
	std::lock_guard lock{ _mtx };
	_pending.push_back(command);
}

void WorldLifecycleBuffer::Enqueue(WorldLifecycleCommand&& command)
{
	std::lock_guard lock{ _mtx };
	_pending.push_back(std::move(command));
}

void WorldLifecycleBuffer::DrainTo(std::vector<WorldLifecycleCommand>& out)
{
	std::lock_guard lock{ _mtx };

	if (_pending.empty())
		return;

	const size_t originalSize = out.size();
	out.resize(originalSize + _pending.size());
	std::move(_pending.begin(), _pending.end(), out.begin() + static_cast<std::ptrdiff_t>(originalSize));
	_pending.clear();
}

void WorldLifecycleBuffer::Clear()
{
	std::lock_guard lock{ _mtx };
	_pending.clear();
}

bool WorldLifecycleBuffer::Empty() const
{
	std::lock_guard lock{ _mtx };
	return _pending.empty();
}
