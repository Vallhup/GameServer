#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "IConnectionAcceptSink.h"
#include "IocpNetworkBackend.h"
#include "PacketDispatchTable.h"
#include "Session.h"
#include "SessionManager.h"

struct IExecutorIOSink;

class NetworkRuntime final : public IConnectionAcceptSink {
public:
	struct Config
	{
		uint16_t workerThreadCount{ 4 };	// IOCP에서는 미사용 (TaskExecutor 관리)
		uint16_t listenPort{ 7000 };
		uint32_t maxSessions{ 5000 };
		uint32_t maxPacketSize{ 1024 * 1024 };
	};

public:
	explicit NetworkRuntime(Config config = {});
	~NetworkRuntime() override;

	NetworkRuntime(const NetworkRuntime&) = delete;
	NetworkRuntime& operator=(const NetworkRuntime&) = delete;

public:
	// IConnectionAcceptSink 구현 — IocpNetworkBackend 가 수락한 연결을 SessionManager 에 등록한다.
	void OnConnectionAccepted(IocpConnection* connection) noexcept override;

	// Initialize() 호출 전에 반드시 설정해야 한다.
	void SetIOSink(IExecutorIOSink& sink) noexcept { _sink = &sink; }

	bool Initialize();
	void Start();
	void Stop() noexcept;
	void Shutdown() noexcept;

	bool IsInitialized() const noexcept;
	bool IsRunning() const noexcept;

	// 패킷 타입 → DynamicTask 타입 매핑 등록
	void RegisterPacketHandler(uint16_t packetType, DynamicTaskTypeId typeId);

	// DrainInboundMessages는 DynamicTask 직접 주입으로 대체되었다.
	// 기존 호출 사이트 호환성을 위해 stub으로 유지한다.
	template<typename T>
	void DrainInboundMessages(T&) noexcept {}

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

	uint32_t GetApproxSessionCount() const noexcept;

	INetworkBackend& GetNetworkBackend() noexcept;
	IocpNetworkBackend& GetIocpBackend() noexcept { return *_backend; }
	SessionManager& GetSessionManager() noexcept { return _sessionManager; }

private:
	Config _config;
	IExecutorIOSink* _sink{ nullptr };

	std::atomic<bool> _initialized{ false };
	std::atomic<bool> _running{ false };

	DefaultPacketDispatchTable _dispatchTable;
	SessionManager _sessionManager;
	std::unique_ptr<IocpNetworkBackend> _backend;
};
