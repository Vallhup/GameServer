#include "pch.h"
#include "Listener.h"

Listener::~Listener()
{
	Stop();
}

bool Listener::Init(short portNum)
{
	_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, NULL);
	if (INVALID_SOCKET == _socket) {
		LOG_ERR("WSASocket failed: %d", WSAGetLastError());
		return false;
	}

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(portNum);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (SOCKET_ERROR == bind(_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(SOCKADDR_IN))) {
		LOG_ERR("bind failed: %d", WSAGetLastError());
		Stop();
		return false;
	}

	if (SOCKET_ERROR == listen(_socket, SOMAXCONN)) {
		LOG_ERR("listen failed: %d", WSAGetLastError());
		Stop();
		return false;
	}

	u_long mode{ 1 };
	ioctlsocket(_socket, FIONBIO, &mode);

	return true;
}

void Listener::Stop()
{
	if (INVALID_SOCKET != _socket) {
		closesocket(_socket);
		_socket = INVALID_SOCKET;
	}
}

SOCKET Listener::GetSocket() const
{
	return _socket;
}

SOCKET Listener::Accept()
{
	return accept(_socket, nullptr, nullptr);
}
