#include "pch.h"
#include "Listener.h"

Listener::Listener(IGameContext& gameCtx) : _gameCtx(gameCtx)
{
	_socket = INVALID_SOCKET;
}

Listener::~Listener()
{
	Stop();
}

bool Listener::Init(int portNum)
{
	_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == _socket) {
		return false;
	}

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(portNum);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (SOCKET_ERROR == bind(_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(SOCKADDR_IN))) {
		return false;
	}

	if (SOCKET_ERROR == listen(_socket, SOMAXCONN)) {
		return false;
	}

	return true;
}

bool Listener::Start()
{
	bool expected{ false };
	if (_accepting.compare_exchange_strong(expected, true)) {
		const unsigned int acceptCount = std::thread::hardware_concurrency() * 2;
		_acceptOvers.reserve(acceptCount);

		for (unsigned int i = 0; i < acceptCount; ++i) {
			AcceptOver* acceptOver = new AcceptOver;
			_acceptOvers.push_back(acceptOver);

			RegisterAccept(acceptOver);
		}

		return true;
	}

	return false;
}

void Listener::Stop()
{
	bool expected{ true };
	if (_accepting.compare_exchange_strong(expected, false)) {
		CancelIoEx(GetHandle(), nullptr);

		for (AcceptOver* acceptOver : _acceptOvers) {
			delete acceptOver;
		}
		_acceptOvers.clear();

		closesocket(_socket);
	}
}

HANDLE Listener::GetHandle() const
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Listener::Dispatch(ExpOver* expOver, int numOfBytes)
{
	if (expOver->CheckOpType(Accept)) {
		AcceptOver* acceptOver = static_cast<AcceptOver*>(expOver);
		ProcessAccept(acceptOver);
	}
}

void Listener::RegisterAccept(AcceptOver* acceptOver)
{
	if (not _accepting.load()) {
		return;
	}

	acceptOver->_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == acceptOver->_socket) {
		return;
	}

	DWORD bytes{ 0 };
	BOOL result = AcceptEx(_socket, acceptOver->_socket, acceptOver->_buffer, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		&bytes, acceptOver);

	if (not result) {
		int error = WSAGetLastError();
		if (error != ERROR_IO_PENDING) {
			// Error Log
			RegisterAccept(acceptOver);
		}
	}	
}

void Listener::ProcessAccept(AcceptOver* acceptOver)
{
	if (not _accepting.load()) {
		return;
	}

	_gameCtx.GetSessionManager().AddSession(acceptOver->_socket);
	RegisterAccept(acceptOver);
}
