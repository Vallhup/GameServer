#include "pch.h"
#include "IocpNetworkBackend.h"

#include "IocpConnection.h"
#include "IocpOperation.h"
#include "IConnectionAcceptSink.h"

#include "IExecutorIOSink.h"

#pragma comment(lib, "ws2_32.lib")

IocpNetworkBackend::IocpNetworkBackend(
	Config cfg,
	IExecutorIOSink& sink,
	IConnectionAcceptSink& acceptSink,
	const DefaultPacketDispatchTable& table)
	: _cfg(cfg)
	, _acceptor(_iocpHandle.GetHandle(), cfg.listenPort, [this](SOCKET s) { OnAccept(s); })
	, _sink(sink)
	, _acceptSink(acceptSink)
	, _table(table)
{
}

bool IocpNetworkBackend::Start() noexcept
{
	return _acceptor.Start();
}

void IocpNetworkBackend::Stop() noexcept
{
	_acceptor.Stop();

	std::unique_lock lock(_connectionMutex);
	for (auto& [id, conn] : _connections)
		conn->Close(SessionCloseReason::ServerShutdown);
}

bool IocpNetworkBackend::DrainCompletions(uint32_t) noexcept
{
	// 이미 도착한 IOCP completion을 비차단으로 모두 빼낸다.
	// timeout = 0 → GetQueuedCompletionStatusEx가 즉시 반환 (non-blocking).
	// blocking sleep은 ExecutorIdleCoordinator가 단독 담당한다.
	OVERLAPPED_ENTRY entries[32];
	ULONG count = 0;

	if (!::GetQueuedCompletionStatusEx(
		_iocpHandle.GetHandle(), entries, 32, &count, 0, FALSE))
	{
		return false;
	}

	bool dispatched = false;
	for (ULONG i = 0; i < count; ++i)
	{
		auto* op = reinterpret_cast<IocpOperation*>(entries[i].lpOverlapped);
		if (op == nullptr) continue;
		bool ok = !FAILED(static_cast<HRESULT>(entries[i].Internal));
		op->dispatch(op, entries[i].dwNumberOfBytesTransferred, ok);
		dispatched = true;
	}
	return dispatched;
}

bool IocpNetworkBackend::Send(SessionId id, std::span<const uint8_t> payload) noexcept
{
	IocpConnection* conn = FindConnection(id);
	if (!conn) return false;

	const auto size = static_cast<uint32_t>(payload.size());
	SendBuffer* buf = SendBufferPool::Get().Acquire(size);
	if (!buf) return false;

	if (!buf->TryAppend(payload.data(), size))
	{
		SendBufferPool::Get().Release(buf);
		return false;
	}

	const bool registered = conn->RegisterSend(buf);
	if (!registered)
	{
		SendBufferPool::Get().Release(buf);
		return false;
	}

	// Outbound sends are staged from the main thread after frame execution too.
	// Wake an executor worker so send completions can clear the connection's
	// chain-flush state and post any buffers queued behind the first WSASend.
	_sink.WakeForExternalIO();
	return true;
}

void IocpNetworkBackend::FlushSend() noexcept
{
	// Sends are initiated immediately by RegisterSend's chain-flushing mechanism.
	// No additional per-frame work required.
}

void IocpNetworkBackend::Disconnect(SessionId id, SessionCloseReason reason) noexcept
{
	IocpConnection* conn = FindConnection(id);
	if (conn)
		conn->Close(reason);
}

uint32_t IocpNetworkBackend::GetSessionCount() const noexcept
{
	std::shared_lock lock(_connectionMutex);
	return static_cast<uint32_t>(_connections.size());
}

void IocpNetworkBackend::OnAccept(SOCKET socket) noexcept
{
	const SessionId id = _nextSessionId.fetch_add(1, std::memory_order_relaxed);

	auto conn = std::make_unique<IocpConnection>(socket, _cfg.maxPacketSize, *this, id);

	if (!_iocpHandle.Register(reinterpret_cast<HANDLE>(socket), 0))
	{
		::closesocket(socket);
		return;
	}

	IocpConnection* rawConn = conn.get();

	{
		std::unique_lock lock(_connectionMutex);
		_connections.emplace(id, std::move(conn));
	}

	_acceptSink.OnConnectionAccepted(rawConn);
	if (!rawConn->RegisterRecv())
		OnConnectionIdle(rawConn);
}

void IocpNetworkBackend::OnConnectionIdle(IocpConnection* conn) noexcept
{
	const SessionId id = conn->GetSessionId();
	const SessionCloseReason reason = conn->GetCloseReason();

	if (_disconnectedTaskTypeId != 0)
	{
		DynamicTaskRequest request{};
		request.typeId = _disconnectedTaskTypeId;
		request.scopeId = 0;
		request.targetKind = DynamicTaskTargetKind::ExplicitScope;
		request.payloadKey = static_cast<uint64_t>(reason);
		request.sessionId = id;
		_sink.SubmitDynamicTask(request);
		_sink.WakeForExternalIO();
	}

	std::unique_lock lock(_connectionMutex);
	_connections.erase(id);
}

SendOp* IocpNetworkBackend::AcquireSendOp() noexcept
{
	return _sendPool.Acquire();
}

void IocpNetworkBackend::ReleaseSendOp(SendOp* op) noexcept
{
	_sendPool.Release(op);
}

IocpConnection* IocpNetworkBackend::FindConnection(SessionId id) const noexcept
{
	std::shared_lock lock(_connectionMutex);
	const auto it = _connections.find(id);
	if (it == _connections.end()) return nullptr;
	return it->second.get();
}
