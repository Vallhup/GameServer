#pragma once

#include <atomic>

#include "IocpOperation.h"
#include "NetRecvBuffer.h"

class IocpNetworkBackend;
struct SendBuffer;

using SessionId = uint32_t;

class IocpConnection {
public:
	IocpConnection() = delete;
	IocpConnection(
		SOCKET socket, uint32_t maxPacketSize,
		IocpNetworkBackend& backend, SessionId sessionId);

	~IocpConnection();

	IocpConnection(const IocpConnection&)				= delete;
	IocpConnection& operator=(const IocpConnection&)	= delete;

	bool RegisterRecv() noexcept;
	bool RegisterSend(SendBuffer* buffer) noexcept;

	void OnRecvComplete(DWORD bytes, bool success) noexcept;
	void OnSendComplete(SendOp* op, bool success) noexcept;

	void Close() noexcept;

	bool		IsClosed()		const noexcept;
	SOCKET		GetSocket()		const noexcept { return _socket; }
	SessionId	GetSessionId()	const noexcept { return _sessionId; }

private:
	void TryFlush() noexcept;

	SOCKET _socket{ INVALID_SOCKET };
	SessionId _sessionId;

	RecvOp _recvOp;
	NetRecvBuffer _recvBuffer;

	std::atomic<uint32_t> _pendingIoCount{ 0 };
	std::atomic<bool> _closed{ false };

	IocpNetworkBackend& _backend;

	concurrency::concurrent_queue<SendBuffer*> _sendQueue;
	std::atomic<bool> _isFlushing{ false };
};

