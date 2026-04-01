#include "pch.h"
#include "GameSendBufferManager.h"

#include <algorithm>

#include "SendBuffer.h"

namespace {
	constexpr uint32_t kDefaultBufferCapacity = 4096;
}

GameSendBufferManager::~GameSendBufferManager()
{
	Reset();
}

void GameSendBufferManager::BeginSendStage() noexcept
{
	// Tick boundary marker. The stage is expected to be empty after FlushSendStage().
}

bool GameSendBufferManager::StageUnicast(
	SessionId sessionId,
	std::span<const uint8_t> payload)
{
	if (sessionId == 0 || payload.empty())
	{
		return false;
	}

	SessionPacketQueue& queue = _sessionQueues[sessionId];
	return AppendToSession(
		queue,
		payload.data(),
		static_cast<uint32_t>(payload.size()));
}

bool GameSendBufferManager::StageMulticast(
	std::span<const SessionId> sessionIds,
	std::span<const uint8_t> payload
)
{
	if (sessionIds.empty() || payload.empty())
	{
		return false;
	}

	SendBuffer* buffer = AcquireBuffer(static_cast<uint32_t>(payload.size()));
	if (buffer == nullptr)
	{
		return false;
	}

	if (!buffer->TryAppend(payload.data(), static_cast<uint32_t>(payload.size())))
	{
		ReleaseBuffer(buffer);
		return false;
	}

	StagedSend stagedSend{};
	stagedSend.buffer = buffer;
	stagedSend.targetSessionIds.assign(sessionIds.begin(), sessionIds.end());
	_multicasts.push_back(std::move(stagedSend));
	return true;
}

void GameSendBufferManager::ReleaseSession(SessionId sessionId) noexcept
{
	const auto it = _sessionQueues.find(sessionId);
	if (it == _sessionQueues.end())
	{
		return;
	}

	ReleaseQueue(it->second);
	_sessionQueues.erase(it);
}

void GameSendBufferManager::Reset() noexcept
{
	for (auto& [sessionId, queue] : _sessionQueues)
	{
		(void)sessionId;
		ReleaseQueue(queue);
	}
	_sessionQueues.clear();

	for (StagedSend& staged : _multicasts)
	{
		ReleaseBuffer(staged.buffer);
	}
	_multicasts.clear();
}

GameSendBufferManager::FlushBatch GameSendBufferManager::FlushSendStage()
{
	FlushBatch batch{};

	for (auto& [sessionId, queue] : _sessionQueues)
	{
		for (SendBuffer*& buffer : queue.completed)
		{
			if (buffer == nullptr || buffer->size == 0)
			{
				ReleaseBuffer(buffer);
				continue;
			}

			StagedSend stagedSend{};
			stagedSend.buffer = buffer;
			stagedSend.targetSessionIds.push_back(sessionId);
			batch.stagedSends.push_back(std::move(stagedSend));
			buffer = nullptr;
		}
		queue.completed.clear();

		if (queue.pending != nullptr)
		{
			if (queue.pending->size > 0)
			{
				StagedSend stagedSend{};
				stagedSend.buffer = queue.pending;
				stagedSend.targetSessionIds.push_back(sessionId);
				batch.stagedSends.push_back(std::move(stagedSend));
			}
			else
			{
				ReleaseBuffer(queue.pending);
			}

			queue.pending = nullptr;
		}
	}
	_sessionQueues.clear();

	for (StagedSend& staged : _multicasts)
	{
		batch.stagedSends.push_back(std::move(staged));
	}
	_multicasts.clear();

	return batch;
}

SendBuffer* GameSendBufferManager::AcquireBuffer(uint32_t minCapacity)
{
	const uint32_t capacity = (std::max)(minCapacity, kDefaultBufferCapacity);
	SendBuffer* buffer = SendBufferPool::Get().Acquire(capacity);
	if (buffer != nullptr)
	{
		buffer->Reset();
	}

	return buffer;
}

void GameSendBufferManager::ReleaseBuffer(SendBuffer*& buffer) noexcept
{
	if (buffer == nullptr)
	{
		return;
	}

	SendBufferPool::Get().Release(buffer);
	buffer = nullptr;
}

void GameSendBufferManager::ReleaseQueue(SessionPacketQueue& queue) noexcept
{
	ReleaseBuffer(queue.pending);

	for (SendBuffer*& buffer : queue.completed)
	{
		ReleaseBuffer(buffer);
	}
	queue.completed.clear();
}

bool GameSendBufferManager::AppendToSession(
	SessionPacketQueue& queue,
	const void* data,
	uint32_t len)
{
	if (data == nullptr || len == 0)
	{
		return false;
	}

	if (queue.pending == nullptr)
	{
		queue.pending = AcquireBuffer(len);
		if (queue.pending == nullptr)
		{
			return false;
		}
	}

	if (queue.pending->TryAppend(data, len))
	{
		return true;
	}

	queue.completed.push_back(queue.pending);
	queue.pending = AcquireBuffer(len);
	if (queue.pending == nullptr)
	{
		return false;
	}

	if (!queue.pending->TryAppend(data, len))
	{
		ReleaseBuffer(queue.pending);
		return false;
	}

	return true;
}
