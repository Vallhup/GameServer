#include "pch.h"
#include "IocpAcceptor.h"

#pragma comment(lib, "Mswsock.lib")

IocpAcceptor::IocpAcceptor(HANDLE iocpHandle, uint16_t port, OnAcceptFn onAccept)
	: _iocpHandle(iocpHandle)
	, _port(port)
	, _onAccept(std::move(onAccept))
{
}

bool IocpAcceptor::Start() noexcept
{
	_listenSocket = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (_listenSocket == INVALID_SOCKET) return false;

	SOCKADDR_IN addr{};
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port        = htons(_port);

	if (::bind(_listenSocket, reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr)) == SOCKET_ERROR)
	{
		::closesocket(_listenSocket);
		_listenSocket = INVALID_SOCKET;
		return false;
	}

	if (::listen(_listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		::closesocket(_listenSocket);
		_listenSocket = INVALID_SOCKET;
		return false;
	}

	if (::CreateIoCompletionPort(
		reinterpret_cast<HANDLE>(_listenSocket), _iocpHandle, 0, 0) == nullptr)
	{
		::closesocket(_listenSocket);
		_listenSocket = INVALID_SOCKET;
		return false;
	}

	// Get AcceptEx function pointer
	GUID guidAcceptEx  = WSAID_ACCEPTEX;
	DWORD bytesReturned = 0;
	if (::WSAIoctl(_listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx, sizeof(guidAcceptEx),
		&_acceptEx, sizeof(_acceptEx),
		&bytesReturned, nullptr, nullptr) == SOCKET_ERROR)
	{
		::closesocket(_listenSocket);
		_listenSocket = INVALID_SOCKET;
		return false;
	}

	_running.store(true, std::memory_order_release);

	for (uint32_t i = 0; i < kPrePostCount; ++i)
	{
		AcceptOp* op = _pool.Acquire();
		if (!op) break;

		SOCKET clientSocket = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP,
			nullptr, 0, WSA_FLAG_OVERLAPPED);
		if (clientSocket == INVALID_SOCKET)
		{
			_pool.Release(op);
			break;
		}

		op->Init(this, clientSocket);
		RegisterAccept(op);
	}

	return true;
}

void IocpAcceptor::Stop() noexcept
{
	_running.store(false, std::memory_order_release);

	if (_listenSocket != INVALID_SOCKET)
	{
		::closesocket(_listenSocket);
		_listenSocket = INVALID_SOCKET;
	}
}

void IocpAcceptor::RegisterAccept(AcceptOp* op) noexcept
{
	if (!_running.load(std::memory_order_acquire)) return;

	DWORD bytesReceived = 0;
	// addrBuf stores local + remote address; receive 0 data bytes on connect
	BOOL result = _acceptEx(
		_listenSocket, op->socket,
		op->addrBuf, 0,
		sizeof(SOCKADDR_IN6) + 16,
		sizeof(SOCKADDR_IN6) + 16,
		&bytesReceived,
		&op->ov);

	if (!result && ::WSAGetLastError() != ERROR_IO_PENDING)
	{
		::closesocket(op->socket);
		op->socket = INVALID_SOCKET;
		_pool.Release(op);
	}
}

void IocpAcceptor::OnAcceptComplete(AcceptOp* op, DWORD bytes, bool success) noexcept
{
	SOCKET clientSocket = op->socket;
	op->socket = INVALID_SOCKET;

	// Re-post a fresh accept before processing to minimize accept latency
	if (_running.load(std::memory_order_acquire))
	{
		AcceptOp* newOp = _pool.Acquire();
		if (newOp)
		{
			SOCKET newSocket = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP,
				nullptr, 0, WSA_FLAG_OVERLAPPED);
			if (newSocket != INVALID_SOCKET)
			{
				newOp->Init(this, newSocket);
				RegisterAccept(newOp);
			}
			else
			{
				_pool.Release(newOp);
			}
		}
	}

	_pool.Release(op);

	if (!success || clientSocket == INVALID_SOCKET)
	{
		if (clientSocket != INVALID_SOCKET)
			::closesocket(clientSocket);
		return;
	}

	if (::setsockopt(clientSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
		reinterpret_cast<char*>(&_listenSocket), sizeof(_listenSocket)) == SOCKET_ERROR)
	{
		::closesocket(clientSocket);
		return;
	}

	_onAccept(clientSocket);
}
