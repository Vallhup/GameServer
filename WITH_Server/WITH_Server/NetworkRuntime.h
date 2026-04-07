#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "GameServerNetworkListener.h"
#include "GameServerNetworkService.h"
#include "Session.h"

class Connection;
struct SendBuffer;

class NetworkRuntime final
	: public IAcceptedConnectionSink
	, public IInboundMessageSink {
public:
	struct Config
	{
		uint16 workerThreadCount{ 4 };
		uint16 listenPort{ 7000 };
		uint32 maxSessions{ 1024 };
		uint32 maxPacketSize{ 1024 * 1024 };
	};

public:
	explicit NetworkRuntime(Config config = {});
	~NetworkRuntime();

	NetworkRuntime(const NetworkRuntime&) = delete;
	NetworkRuntime& operator=(const NetworkRuntime&) = delete;

public:
	bool Initialize();
	void Start();
	void Stop() noexcept;
	void Shutdown() noexcept;

	bool IsInitialized() const noexcept;
	bool IsRunning() const noexcept;

	void DrainInboundMessages(std::vector<InboundMessage>& outMessages);

	void BeginSendStage() noexcept;

	bool StageUnicast(SessionId sessionId, std::span<const uint8_t> payload);
	bool StageMulticast(
		std::span<const SessionId> sessionIds,
		std::span<const uint8_t> payload);

	bool StageBroadcastAll(std::span<const uint8_t> payload);
	bool StageBroadcastExcept(
		SessionId exceptSessionId,
		std::span<const uint8_t> payload);
	bool StageBroadcastExceptMany(
		std::span<const SessionId> exceptSessionIds,
		std::span<const uint8_t> payload);

	void FlushSendStage();

	bool RequestCompleteLogin(SessionId sessionId);
	bool RequestEnterInGame(SessionId sessionId, NetId playerNetId);
	bool RequestLeaveGame(SessionId sessionId);
	bool RequestClose(SessionId sessionId, SessionCloseReason reason);

	uint32 GetApproxSessionCount() const noexcept;

private:
	void OnAcceptedConnection(
		const std::shared_ptr<Connection>& connection) override;

	void OnInboundMessage(InboundMessage&& message) override;

private:
	struct Impl;

	Config _config;
	std::unique_ptr<Impl> _impl;
};
