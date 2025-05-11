#include "pch.h"
#include "Listener.h"

Listener::~Listener()
{
	StopAccept();

	for (AcceptOver* acceptOver : _acceptOvers) {
		delete acceptOver;
	}
	_acceptOvers.clear();

	CloseSocket();
}

bool Listener::StartAccept(std::shared_ptr<Service> service)
{
	std::cout << "StartAccept Listener\n";

	_service = service;
	if (nullptr == _service.lock()) {
		std::cout << "_service Error Listener\n";
		return false;
	}
		

	_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == _socket) {
		std::cout << "_socket Error Listener\n";
		return false;
	}
		

	// IocpCore에 ListenSocket 등록
	if (false == service->getIocpCore()->Register(shared_from_this())) {
		std::cout << "Register Error Listener\n";
		return false;
	}

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// Socket Bind
	if (SOCKET_ERROR == bind(_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(SOCKADDR_IN))) {
		std::cout << "bind Error\n";
		return false;
	}

	// Socket Listen
	if(listen(_socket, SOMAXCONN)) {
		std::cout << "listen Error\n";
		return false;
	}

	_accepting.store(true);

	const int acceptCount = service->getMaxSessionCount();
	_acceptOvers.reserve(acceptCount);
	for (int i = 0; i < acceptCount; ++i) {
		AcceptOver* acceptOver = new AcceptOver;
		acceptOver->_owner = shared_from_this();
		_acceptOvers.push_back(acceptOver);

		doAccept(acceptOver);
	}

	
	return true;
}

void Listener::StopAccept()
{
	_accepting = false;

	for (AcceptOver* acceptOver : _acceptOvers) {
		CancelIoEx(reinterpret_cast<HANDLE>(_socket), reinterpret_cast<LPOVERLAPPED>(acceptOver));
	}
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
	std::cout << "Dispatch Listener\n";

	if (expOver->_operationType == OperationType::Accept) {
		AcceptOver* acceptOver = static_cast<AcceptOver*>(expOver);
		AcceptCallback(acceptOver);
	}
}

void Listener::doAccept(AcceptOver* acceptOver)
{
	std::cout << "Start doAccept Listener\n";

	if (not _accepting.load()) {
		return;
	}

	std::shared_ptr<GameSession> session = std::make_shared<GameSession>();

	acceptOver->Init();
	acceptOver->_session = session;

	DWORD bytesReceived{ 0 };
	BOOL result = AcceptEx(_socket, session->GetSocket(), acceptOver->_buffer, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		&bytesReceived, static_cast<LPOVERLAPPED>(acceptOver));

	if (result == FALSE) {
		int error = WSAGetLastError();
		if (error != ERROR_IO_PENDING) {
			std::cout << "AcceptEx Error : " << error << std::endl;
		}

		else {
			std::cout << "AcceptEx pending\n";
		}
	}

	else {
		std::cout << "AcceptEx Success\n";
	}
}

void Listener::AcceptCallback(AcceptOver* acceptOver)
{
	std::cout << "Start AcceptCallback Listener\n";

	if (not _accepting.load()) {
		return;
	}

	ServicePtr service = _service.lock();
	if (not service) {
		return;
	}

	// 어떤 Session이 Accept했는지 확인
	std::shared_ptr<GameSession> session = static_pointer_cast<GameSession>(acceptOver->_session);

	setsockopt(session->GetSocket(), SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&_socket, sizeof(_socket));

	// session의 socket을 IocpCore에 등록
	service->getIocpCore()->Register(session);

	// session을 User를 관리하는 container에 push
	service->AddSession(session);

	// session Recv 시작
	session->doRecv();

	doAccept(acceptOver);
}
