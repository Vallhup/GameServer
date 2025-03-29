#include "ServerCore.h"

bool ServerCore::Init(short serverPort)
{
	WSADATA WSAData;
	if (not WSAStartup(MAKEWORD(2, 2), &WSAData)) {
		std::cout << "WSAStartup Error";
		return false;
	}

	_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == _listenSocket) {
		std::cout << "WSASocket Error";
		return false;
	}

	SOCKADDR_IN clientAddr;
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(serverPort);
	clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (SOCKET_ERROR == bind(_listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), sizeof(SOCKADDR_IN))) {
		std::cout << "bind Error";
		return false;
	}

	if (SOCKET_ERROR == listen(_listenSocket, SOMAXCONN)) {
		std::cout << "listen Error";
		return false;
	}

	return true;
}

void ServerCore::MainLoop()
{
	while (true) {
		ClientAccept();
	}
}

void ServerCore::ClientAccept()
{
	SOCKADDR_IN clientAddr;
	INT addrSize = sizeof(SOCKADDR_IN);

	SOCKET clientSocket = WSAAccept(_listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &addrSize, NULL, NULL);
	if (INVALID_SOCKET == clientSocket) {
		std::cout << "WSAAccept Error";
		return;
	}

	std::unique_ptr<Session> session = std::make_unique<Session>(_sessionId++, clientSocket);
	_sessions[session->GetSessionId()] = std::move(session);

	// 굳이 unique_ptr로 만들어 놓고 왜 일반포인터?
	// 
	// 1. sessions의 session과 ExpOver의 session을 독립적으로 동작하게하기 위해
	//    (ExpOver의 session은 아직 비동기 IO 작업을 수행중인데 sessions에서는 session을 지워버릴 수 있음)
	// 
	// 2. session 삭제 시 안전성 관리 
	//    (sessions에서 session을 삭제할 때 session을 참조하는 ExpOver를 먼저 정리해줄 수 있음)
	// 
	// 3. 쓰기 편함

	// 위의 복합적 이유로 비동기 IO작업에는 일반포인터를 사용하는 것이 좋다!
	// Smart Pointer는 객체의 소유권 및 생명주기 관리 용도로만 사용하는 게 좋다
	Session* sessionPtr = session.get();
	RecvCall(sessionPtr);
}

void ServerCore::RecvCall(Session* session) const
{
	ExpOver* recvOver = new ExpOver(session);
}

void ServerCore::SendCall(Session* session, const Packet& packet) const
{
}

void ServerCore::RecvCallback(DWORD error, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag)
{
}

void ServerCore::SendCallback(DWORD error, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag)
{
}
