#include "pch.h"
#include "Listener.h"

Listener::Listener(const std::shared_ptr<Service>& service) : _service(service)
{
}

Listener::~Listener()
{
	closesocket(_socket);
}

bool Listener::Init()
{
	auto service = _service.lock();
	if (nullptr == service) {
		LOG_ERR("StartAccept failed: service is nullptr");
		return false;
	}

	_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, NULL);
	if (INVALID_SOCKET == _socket) {
		LOG_ERR("WSASocket failed: %d", WSAGetLastError());
		return false;
	}

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT_NUM);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (SOCKET_ERROR == bind(_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(SOCKADDR_IN))) {
		LOG_ERR("bind failed: %d", WSAGetLastError());
		closesocket(_socket);
		return false;
	}

	if (SOCKET_ERROR == listen(_socket, SOMAXCONN)) {
		LOG_ERR("listen failed: %d", WSAGetLastError());
		closesocket(_socket);
		return false;
	}

	u_long mode{ 1 };
	ioctlsocket(_socket, FIONBIO, &mode);

	return true;
}

SOCKET Listener::GetSocket() const
{
	return _socket;
}

SOCKET Listener::Accept()
{
	return accept(_socket, nullptr, nullptr);
}
