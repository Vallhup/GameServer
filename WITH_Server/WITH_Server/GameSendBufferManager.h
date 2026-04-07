#pragma once

#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

#include "Session.h"

struct SendBuffer;

class GameSendBufferManager final {
public:
	struct StagedSend
	{
		std::vector<SessionId> targetSessionIds;
		SendBuffer* buffer{ nullptr };
	};

	struct FlushBatch
	{
		std::vector<StagedSend> stagedSends;
	};

public:
	GameSendBufferManager() = default;
	~GameSendBufferManager();

	GameSendBufferManager(const GameSendBufferManager&) = delete;
	GameSendBufferManager& operator=(const GameSendBufferManager&) = delete;
	GameSendBufferManager(GameSendBufferManager&&) = delete;
	GameSendBufferManager& operator=(GameSendBufferManager&&) = delete;

public:
	void BeginSendStage() noexcept;

	bool StageUnicast(SessionId sessionId, std::span<const uint8_t> payload);
	bool StageMulticast(
		std::span<const SessionId> sessionIds,
		std::span<const uint8_t> payload);

	void ReleaseSession(SessionId sessionId) noexcept;
	void Reset() noexcept;

	FlushBatch FlushSendStage();

private:
	struct SessionPacketQueue
	{
		SendBuffer* pending{ nullptr };
		std::vector<SendBuffer*> completed;
	};

	SendBuffer* AcquireBuffer(uint32_t minCapacity);
	void ReleaseBuffer(SendBuffer*& buffer) noexcept;
	void ReleaseQueue(SessionPacketQueue& queue) noexcept;
	bool AppendToSession(
		SessionPacketQueue& queue,
		const void* data,
		uint32_t len);

private:
	std::unordered_map<SessionId, SessionPacketQueue> _sessionQueues;
	std::vector<StagedSend> _multicasts;
};
