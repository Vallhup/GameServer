#include "pch.h"
#include "SessionSendBufferManager.h"
#include "Framework.h"

void SessionSendBufferManager::Enqueue(uint32_t connId, const void* data, uint32_t len)
{
	assert(connId != std::numeric_limits<uint32>::max());

	if (!data || len == 0) return;
	if (connId >= _buffers.size())
	{
		_buffers.resize(connId + 1);
	}

	auto& buffer = _buffers[connId];

	if (!buffer)
	{
		buffer = SendBufferPool::Get().Acquire(std::max(len, 4096u));
	}

	if (!buffer->TryAppend(data, len))
	{
		FlushOne(connId);

		buffer = SendBufferPool::Get().Acquire(std::max(len, 4096u));
		buffer->TryAppend(data, len);
	}
}

void SessionSendBufferManager::FlushOne(uint32_t connId)
{
	if (connId >= _buffers.size()) return;
	auto& buffer = _buffers[connId];

	if (!buffer) return;
	if (buffer->size == 0)
	{
		buffer = nullptr;
		return;
	}

	_listener.Send(connId, buffer);
	buffer = nullptr;
}

void SessionSendBufferManager::FlushAll()
{
	for (uint32_t i = 0; i < (uint32_t)_buffers.size(); ++i)
	{
		if (!_buffers[i]) continue;
		FlushOne(i);
	}
}

void SessionSendBufferManager::ReleaseSession(uint32_t connId)
{
	if (connId >= _buffers.size()) return;
	_buffers[connId] = nullptr;
}