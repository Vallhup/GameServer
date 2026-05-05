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
		conn->Close();
}

void IocpNetworkBackend::WaitForWork(
	uint32_t, std::chrono::microseconds timeout) noexcept
{
	OVERLAPPED_ENTRY entries[32];
	ULONG count = 0;
	DWORD ms = static_cast<DWORD>(
		std::clamp<int64_t>(timeout.count() / 1000, 0, INFINITE - 1));

	::GetQueuedCompletionStatusEx(
		_iocpHandle.GetHandle(), entries, 32, &count, ms, FALSE);

	for (ULONG i = 0; i < count; ++i)
	{
		auto* op = reinterpret_cast<IocpOperation*>(entries[i].lpOverlapped);
		if (op == nullptr) continue;
		bool ok = !FAILED(static_cast<HRESULT>(entries[i].Internal));
		op->dispatch(op, entries[i].dwNumberOfBytesTransferred, ok);
	}
}

void IocpNetworkBackend::WakeWorker() noexcept
{
	_iocpHandle.WakeWorker();
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

	return conn->RegisterSend(buf);
}

void IocpNetworkBackend::FlushSend() noexcept
{
	// Sends are initiated immediately by RegisterSend's chain-flushing mechanism.
	// No additional per-frame work required.
}

void IocpNetworkBackend::Disconnect(SessionId id) noexcept
{
	IocpConnection* conn = FindConnection(id);
	if (conn)
		conn->Close();
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
	rawConn->RegisterRecv();
}

void IocpNetworkBackend::OnConnectionIdle(IocpConnection* conn) noexcept
{
	const SessionId id = conn->GetSessionId();

	if (_disconnectedTaskTypeId != 0)
	{
		DynamicTaskRequest request{};
		request.typeId = _disconnectedTaskTypeId;
		request.scopeId = 0;
		request.sessionId = id;
		_sink.SubmitDynamicTask(request);
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
