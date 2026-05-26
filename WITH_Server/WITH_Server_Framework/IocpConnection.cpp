#include "pch.h"
#include "IocpConnection.h"
#include "IocpNetworkBackend.h"
#include "Protocol.h"

#include "FrameworkLog.h"
#include "IExecutorIOSink.h"

#include <cstring>

namespace
{
	constexpr const char* kLogCategory = "IocpConnection";

	bool TryReadPacketHeader(
		const SendBuffer* buffer,
		PacketHeader& outHeader) noexcept
	{
		if (buffer == nullptr ||
			buffer->data == nullptr ||
			buffer->size < sizeof(PacketHeader))
		{
			return false;
		}

		std::memcpy(&outHeader, buffer->data, sizeof(PacketHeader));
		return true;
	}
}

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
		Close(SessionCloseReason::RemoteClosed);
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
		Close(SessionCloseReason::RemoteClosed);
		return false;
	}

	return true;
}

bool IocpConnection::RegisterSend(SendBuffer* buffer) noexcept
{
	if (IsClosed()) return false;

	PacketHeader header{};
	if (TryReadPacketHeader(buffer, header))
	{
		FWLOG_INFO(kLogCategory,
			"SendRegister (sid=%u, packetSize=%u, packetType=%u, bufferSize=%u)",
			_sessionId,
			header.size,
			header.type,
			buffer->size);
	}
	else
	{
		FWLOG_WARN(kLogCategory,
			"SendRegister with invalid buffer (sid=%u, bufferSize=%u)",
			_sessionId,
			buffer != nullptr ? buffer->size : 0u);
	}

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

	uint32_t totalBytes = 0;
	uint32_t firstPacketSize = 0;
	uint32_t firstPacketType = 0;
	for (const SendBuffer* owned : op->ownedBuffers)
	{
		totalBytes += owned != nullptr ? owned->size : 0u;
		if (firstPacketSize == 0)
		{
			PacketHeader header{};
			if (TryReadPacketHeader(owned, header))
			{
				firstPacketSize = header.size;
				firstPacketType = header.type;
			}
		}
	}
	FWLOG_INFO(kLogCategory,
		"SendFlush posting WSASend (sid=%u, bufferCount=%zu, totalBytes=%u, firstPacketSize=%u, firstPacketType=%u)",
		_sessionId,
		op->ownedBuffers.size(),
		totalBytes,
		firstPacketSize,
		firstPacketType);

	DWORD bytesSent = 0;
	int result = ::WSASend(
		_socket,
		op->wsaBufs.data(),
		static_cast<DWORD>(op->wsaBufs.size()),
		&bytesSent, 0,
		&op->ov,
		nullptr);

	const int sendError =
		result == SOCKET_ERROR ? ::WSAGetLastError() : 0;
	if (result == SOCKET_ERROR && sendError != WSA_IO_PENDING)
	{
		FWLOG_WARN(kLogCategory,
			"SendFlush WSASend failed (sid=%u, error=%d)",
			_sessionId,
			sendError);
		_pendingIoCount.fetch_sub(1, std::memory_order_acq_rel);
		_backend.ReleaseSendOp(op);
		_isFlushing.store(false, std::memory_order_release);
		Close(SessionCloseReason::RemoteClosed);
	}
}

void IocpConnection::OnRecvComplete(DWORD bytes, bool success) noexcept
{
	auto done = [this]()
	{
		if (_pendingIoCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
			_backend.OnConnectionIdle(this);
	};

	FWLOG_INFO(kLogCategory,
		"RecvComplete (sid=%u, bytes=%lu, success=%u, closed=%u)",
		_sessionId,
		static_cast<unsigned long>(bytes),
		success ? 1u : 0u,
		IsClosed() ? 1u : 0u);

	if (!success || bytes == 0 || !_recvBuffer.CommitWrite(bytes))
	{
		FWLOG_WARN(kLogCategory,
			"RecvComplete closing session (sid=%u, bytes=%lu, success=%u)",
			_sessionId,
			static_cast<unsigned long>(bytes),
			success ? 1u : 0u);
		Close(SessionCloseReason::RemoteClosed);
		done();
		return;
	}

	while (true)
	{
		auto packet = _recvBuffer.PeekPacket();
		if (packet.empty()) break;

		const auto* header = reinterpret_cast<const PacketHeader*>(packet.data());
		DynamicTaskTypeId typeId = _backend.GetDispatchTable().Lookup(header->type);

		FWLOG_INFO(kLogCategory,
			"RecvPacket peeked (sid=%u, packetSize=%u, packetType=%u, taskTypeId=%u)",
			_sessionId,
			header->size,
			header->type,
			typeId);

		if (typeId == InvalidDynamicTaskTypeId)
		{
			FWLOG_WARN(kLogCategory,
				"RecvPacket invalid packet type - closing (sid=%u, packetSize=%u, packetType=%u)",
				_sessionId,
				header->size,
				header->type);
			Close(SessionCloseReason::ProtocolError);
			done();
			return;
		}

		SendBufferPtr buf(SendBufferPool::Get().Acquire(header->size));
		if (!buf)
		{
			Close(SessionCloseReason::RemoteClosed);
			done();
			return;
		}
		buf->TryAppend(packet.data(), header->size);

		DynamicTaskRequest request{};
		request.typeId = typeId;
		request.scopeId = 0;
		request.targetKind = DynamicTaskTargetKind::TypeDefault;
		request.payloadKey = reinterpret_cast<uint64_t>(buf.release());
		request.sessionId = _sessionId;
		_backend.GetIOSink().SubmitDynamicTask(request);

		FWLOG_INFO(kLogCategory,
			"RecvPacket submitted dynamic task (sid=%u, packetType=%u, taskTypeId=%u, payloadSize=%u)",
			_sessionId,
			header->type,
			typeId,
			header->size);

		_recvBuffer.ConsumePacket(header->size);
	}

	if (!IsClosed())
	{
		PacketHeader pendingHeader{};
		const uint32_t available = _recvBuffer.AvailableBytes();
		if (available > 0)
		{
			if (_recvBuffer.TryPeekHeader(pendingHeader))
			{
				FWLOG_INFO(kLogCategory,
					"RecvBuffer pending bytes after drain (sid=%u, available=%u, pendingSize=%u, pendingType=%u)",
					_sessionId,
					available,
					pendingHeader.size,
					pendingHeader.type);
			}
			else
			{
				FWLOG_INFO(kLogCategory,
					"RecvBuffer pending partial header after drain (sid=%u, available=%u)",
					_sessionId,
					available);
			}
		}

		const bool recvRegistered = RegisterRecv();
		FWLOG_INFO(kLogCategory,
			"Recv re-register result (sid=%u, ok=%u, closed=%u)",
			_sessionId,
			recvRegistered ? 1u : 0u,
			IsClosed() ? 1u : 0u);
	}

	done();
}

void IocpConnection::OnSendComplete(SendOp* op, bool success) noexcept
{
	uint32_t totalBytes = 0;
	uint32_t firstPacketSize = 0;
	uint32_t firstPacketType = 0;
	for (const SendBuffer* buf : op->ownedBuffers)
	{
		totalBytes += buf != nullptr ? buf->size : 0u;
		if (firstPacketSize == 0)
		{
			PacketHeader header{};
			if (TryReadPacketHeader(buf, header))
			{
				firstPacketSize = header.size;
				firstPacketType = header.type;
			}
		}
	}
	FWLOG_INFO(kLogCategory,
		"SendComplete (sid=%u, success=%u, bufferCount=%zu, totalBytes=%u, firstPacketSize=%u, firstPacketType=%u)",
		_sessionId,
		success ? 1u : 0u,
		op->ownedBuffers.size(),
		totalBytes,
		firstPacketSize,
		firstPacketType);

	for (SendBuffer* buf : op->ownedBuffers)
		SendBufferPool::Get().Release(buf);

	_backend.ReleaseSendOp(op);

	if (!success)
	{
		FWLOG_WARN(kLogCategory,
			"SendComplete closing session (sid=%u)",
			_sessionId);
		Close(SessionCloseReason::RemoteClosed);
	}

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

void IocpConnection::Close(SessionCloseReason reason) noexcept
{
	bool expected = false;
	if (!_closed.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
	{
		FWLOG_INFO(kLogCategory,
			"Close skipped (already closed) (sid=%u, requestedReason=%d, existingReason=%d)",
			_sessionId,
			static_cast<int>(reason),
			static_cast<int>(_closeReason.load(std::memory_order_acquire)));
		return;
	}

	_closeReason.store(reason, std::memory_order_release);
	FWLOG_WARN(kLogCategory,
		"Close session (sid=%u, reason=%d)",
		_sessionId,
		static_cast<int>(reason));

	const SOCKET socket = _socket;
	if (socket != INVALID_SOCKET)
	{
		::shutdown(socket, SD_BOTH);
		::closesocket(socket);
		_socket = INVALID_SOCKET;
	}
}

bool IocpConnection::IsClosed() const noexcept
{
	return _closed.load(std::memory_order_acquire);
}

SessionCloseReason IocpConnection::GetCloseReason() const noexcept
{
	const SessionCloseReason reason =
		_closeReason.load(std::memory_order_acquire);
	return reason != SessionCloseReason::None
		? reason
		: SessionCloseReason::RemoteClosed;
}
