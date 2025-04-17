#include "pch.h"
#include "Listener.h"

Listener::~Listener()
{
	closesocket(_socket);

	for (AcceptOver* acceptOver : _acceptOvers) {
		delete acceptOver;
	}
}

bool Listener::StartAccept(std::shared_ptr<ServerService> service)
{
	_service = service;
	if (nullptr == _service)
		return false;

	_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == _socket)
		return false;

	// TODO : IocpCore에 ListenSocket 등록
	{
		// service->iocpCore->Register(shared_from_this());
	}

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// Socket Bind
	bind(_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(SOCKADDR_IN));

	// Socket Listen
	listen(_socket, SOMAXCONN);

	// temp : 서버의 최대 동접을 정해줄 예정 (Service에서)
	const int acceptCount = 10;
	for (int i = 0; i < acceptCount; ++i) {
		AcceptOver* acceptOver = new AcceptOver;
		acceptOver->_owner = shared_from_this();
		_acceptOvers.push_back(acceptOver);

		doAccept(acceptOver);
	}

	return true;
}

void Listener::CloseSocket()
{
	closesocket(_socket);
}

HANDLE Listener::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Listener::Dispatch(ExpOver* expOver, int numOfBytes)
{
	if (expOver->_operationType == OperationType::Accept) {
		AcceptOver* acceptOver = static_cast<AcceptOver*>(expOver);
		AcceptCallback(acceptOver);
	}
}

void Listener::doAccept(AcceptOver* acceptOver)
{
	std::shared_ptr<Session> session = std::make_shared<Session>();

	acceptOver->Init();
	acceptOver->_session = session;

	AcceptEx(_socket, session->GetSocket(), acceptOver->_buffer, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		NULL, static_cast<LPOVERLAPPED>(acceptOver));
}

void Listener::AcceptCallback(AcceptOver* acceptOver)
{
	// 어떤 Session이 Accept했는지 확인
	std::shared_ptr<Session> session = acceptOver->_session;

	// session의 socket을 IocpCore에 등록
	_service->getIocpCore()->Register(session);

	// TODO : session을 User를 관리하는 container에 push
	_service->AddSession(session);

	doAccept(acceptOver);
}
