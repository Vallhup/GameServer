#include "pch.h"
#include "IocpConnection.h"
#include "IocpNetworkBackend.h"
#include "Protocol.h"

#include "IExecutorIOSink.h"

IocpConnection::IocpConnection(
	SOCKET socket, uint32_t maxPacketSize,
	IocpNetworkBackend& backend, SessionId sessionId)
	: _socket(socket)
	, _sessionId(sessionId)
	, _recvBuffer(maxPacketSize * 2)
	, _backend(backend)
{
	_recvOp.Init(this);
}

IocpConnection::~IocpConnection()
{
	if (_socket != INVALID_SOCKET)
		::closesocket(_socket);
}

bool IocpConnection::RegisterRecv() noexcept
{
	if (IsClosed()) return false;

	_pendingIoCount.fetch_add(1, std::memory_order_acq_rel);

	int count = _recvBuffer.PrepareWsaBufs(_recvOp.wsaBufs);
	if (count == 0)
	{
		_pendingIoCount.fetch_sub(1, std::memory_order_acq_rel);
		Close();
		return false;
	}

	DWORD flags = 0;
	DWORD bytesRecv = 0;
	int result = ::WSARecv(
		_socket,
		_recvOp.wsaBufs,
		static_cast<DWORD>(count),
		&bytesRecv, &flags,
		&_recvOp.ov,
		nullptr);

	if (result == SOCKET_ERROR && ::WSAGetLastError() != WSA_IO_PENDING)
	{
		_pendingIoCount.fetch_sub(1, std::memory_order_acq_rel);
		Close();
		return false;
	}

	return true;
}

bool IocpConnection::RegisterSend(SendBuffer* buffer) noexcept
{
	if (IsClosed()) return false;

	_sendQueue.push(buffer);

	bool expected = false;
	if (_isFlushing.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
		TryFlush();

	return true;
}

void IocpConnection::TryFlush() noexcept
{
	SendOp* op = _backend.AcquireSendOp();
	if (!op)
	{
		_isFlushing.store(false, std::memory_order_release);
		return;
	}
	op->Init(this);

	SendBuffer* buf;
	while (_sendQueue.try_pop(buf))
	{
		op->wsaBufs.push_back({ buf->size, reinterpret_cast<char*>(buf->data) });
		op->ownedBuffers.push_back(buf);
	}

	if (op->wsaBufs.empty())
	{
		_backend.ReleaseSendOp(op);
		_isFlushing.store(false, std::memory_order_release);
		return;
	}

	_pendingIoCount.fetch_add(1, std::memory_order_acq_rel);

	DWORD bytesSent = 0;
	int result = ::WSASend(
		_socket,
		op->wsaBufs.data(),
		static_cast<DWORD>(op->wsaBufs.size()),
		&bytesSent, 0,
		&op->ov,
		nullptr);

	if (result == SOCKET_ERROR && ::WSAGetLastError() != WSA_IO_PENDING)
	{
		_pendingIoCount.fetch_sub(1, std::memory_order_acq_rel);
		_backend.ReleaseSendOp(op);
		_isFlushing.store(false, std::memory_order_release);
		Close();
	}
}

void IocpConnection::OnRecvComplete(DWORD bytes, bool success) noexcept
{
	auto done = [this]()
	{
		if (_pendingIoCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
			_backend.OnConnectionIdle(this);
	};

	if (!success || bytes == 0 || !_recvBuffer.CommitWrite(bytes))
	{
		Close();
		done();
		return;
	}

	while (true)
	{
		auto packet = _recvBuffer.PeekPacket();
		if (packet.empty()) break;

		const auto* header = reinterpret_cast<const PacketHeader*>(packet.data());
		DynamicTaskTypeId typeId = _backend.GetDispatchTable().Lookup(header->type);

		if (typeId == InvalidDynamicTaskTypeId)
		{
			Close();
			done();
			return;
		}

		SendBufferPtr buf(SendBufferPool::Get().Acquire(header->size));
		if (!buf)
		{
			Close();
			done();
			return;
		}
		buf->TryAppend(packet.data(), header->size);

		DynamicTaskRequest request{};
		request.typeId = typeId;
		request.scopeId = 0;
		request.payloadKey = reinterpret_cast<uint64_t>(buf.release());
		request.sessionId = _sessionId;
		_backend.GetIOSink().SubmitDynamicTask(request);

		_recvBuffer.ConsumePacket(header->size);
	}

	if (!IsClosed())
		RegisterRecv();

	done();
}

void IocpConnection::OnSendComplete(SendOp* op, bool success) noexcept
{
	for (SendBuffer* buf : op->ownedBuffers)
		SendBufferPool::Get().Release(buf);

	_backend.ReleaseSendOp(op);

	if (!success)
		Close();

	// seq_cst fence: ensures items pushed to _sendQueue before this point are visible
	_isFlushing.store(false, std::memory_order_release);
	std::atomic_thread_fence(std::memory_order_seq_cst);

	bool expected = false;
	if (!IsClosed() &&
		!_sendQueue.empty() &&
		_isFlushing.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
	{
		TryFlush();
	}

	if (_pendingIoCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
		_backend.OnConnectionIdle(this);
}

void IocpConnection::Close() noexcept
{
	bool expected = false;
	if (!_closed.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
		return;

	::shutdown(_socket, SD_BOTH);
	::closesocket(_socket);
	_socket = INVALID_SOCKET;
}

bool IocpConnection::IsClosed() const noexcept
{
	return _closed.load(std::memory_order_acquire);
}
