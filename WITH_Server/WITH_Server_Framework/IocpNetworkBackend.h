#pragma once

#include <shared_mutex>

#include "INetworkBackend.h"

#include "IocpHandle.h"
#include "IocpAcceptor.h"
#include "IocpConnection.h"

#include "ObjectPool.hpp"
#include "PacketDispatchTable.h"


struct IConnectionAcceptSink;

struct IExecutorIOSink;

class IocpNetworkBackend final : public INetworkBackend {
public:
	struct Config
	{
		uint16_t listenPort{ 7000 };
		uint32_t maxSessions{ 5000 };
		uint32_t maxPacketSize{ 1024 * 1024 };
	};

	IocpNetworkBackend() = delete;
	IocpNetworkBackend(
		Config cfg,
		IExecutorIOSink& sink,
		IConnectionAcceptSink& acceptSink,
		const DefaultPacketDispatchTable& table);

	~IocpNetworkBackend() = default;

	IocpNetworkBackend(const IocpNetworkBackend&)				= delete;
	IocpNetworkBackend& operator=(const IocpNetworkBackend&)	= delete;

public:
	// IIOBackend
	[[nodiscard]]
	virtual bool DrainCompletions(uint32_t workerIdx) noexcept override;
	[[nodiscard]]
	virtual const char* DebugName() const noexcept override { return "Network"; }

	// INetworkBackend
	virtual bool Send(SessionId id, std::span<const uint8_t> payload) noexcept override;
	virtual void FlushSend() noexcept override;

	virtual void Disconnect(
		SessionId id,
		SessionCloseReason reason = SessionCloseReason::LocalRequested) noexcept override;

	virtual uint32_t GetSessionCount() const noexcept override;

public:
	bool Start() noexcept;
	void Stop() noexcept;

	void OnAccept(SOCKET socket) noexcept;
	void OnConnectionIdle(IocpConnection* conn) noexcept;

	IExecutorIOSink&                  GetIOSink()          noexcept { return _sink; }
	const DefaultPacketDispatchTable& GetDispatchTable()   noexcept { return _table; }

	SendOp* AcquireSendOp() noexcept;
	void    ReleaseSendOp(SendOp* op) noexcept;

	void SetDisconnectedTaskTypeId(uint32_t typeId) noexcept { _disconnectedTaskTypeId = typeId; }

private:
	IocpConnection* FindConnection(SessionId id) const noexcept;

	Config		_cfg;

	IocpHandle		_iocpHandle;
	IocpAcceptor	_acceptor;

	IExecutorIOSink&                  _sink;
	IConnectionAcceptSink&            _acceptSink;
	const DefaultPacketDispatchTable& _table;

	mutable std::shared_mutex _connectionMutex;
	std::unordered_map<SessionId, std::unique_ptr<IocpConnection>> _connections;

	ObjectPool<SendOp, 2048, OverflowPolicy_Heap<SendOp>> _sendPool;

	std::atomic<SessionId> _nextSessionId{ 1 };

	// 0 == 미등록 (InvalidDynamicTaskTypeId와 동일)
	uint32_t _disconnectedTaskTypeId{ 0 };
};
