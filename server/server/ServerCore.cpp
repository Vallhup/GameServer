#include <iostream>

#include "ServerCore.h"

ServerCore* gServerCore = new ServerCore;

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

	WSACleanup();
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

	_sessions[session->GetSessionId()] = std::move(session);

	ConnectRecvCall(sessionPtr->GetSocket());
}

void ServerCore::ConnectRecvCall(SOCKET clientSocket)
{
	ExpOver* recvOver = new ExpOver(nullptr);
	DWORD recvFlag = 0;

	auto ret = WSARecv(clientSocket, recvOver->GetWsabuf(), 1, NULL, &recvFlag, recvOver->GetOverPtr(), ConnectRecvCallback);
	if (SOCKET_ERROR == ret) {
		auto error = WSAGetLastError();
		if (WSA_IO_PENDING != error) {
			errorDisplay("WSARecv : ", error);
			delete recvOver;
		}
	}
}

void ServerCore::RecvCall(Session* session) const
{
	ExpOver* recvOver = new ExpOver(session);
	DWORD recvFlag = 0;

	auto ret = WSARecv(session->GetSocket(), recvOver->GetWsabuf(), 1, NULL, &recvFlag, recvOver->GetOverPtr(), RecvCallback);
	if (SOCKET_ERROR == ret) {
		auto error = WSAGetLastError();
		if (WSA_IO_PENDING != error) {
			errorDisplay("WSARecv : ", error);
			delete recvOver;
		}
	}
}

void ServerCore::SendCall(Session* session, const Packet& packet) const
{
	ExpOver* sendOver = new ExpOver(session, packet);
	DWORD sentBytes = 0;

	WSASend(session->GetSocket(), sendOver->GetWsabuf(), 1, &sentBytes, 0, sendOver->GetOverPtr(), SendCallback);
}

void ServerCore::ConnectRecvCallback(DWORD error, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag)
{
	ExpOver* recvOver = reinterpret_cast<ExpOver*>(pOver);
	Packet* recvPacket = reinterpret_cast<Packet*>(recvOver->GetBuffer());

	// 최초 받은 Packet이 Connect Packet이면 (정상적인 접속이면)
	if (PACKET_CONNECT == recvPacket->GetType()) {
		ConnectPacket* connectPacket = 
			new ConnectPacket(recvOver->GetSession()->GetSessionId(), recvOver->GetSession()->GetPos());

		// 최초 Position정보 Send 후
		gServerCore->SendCall(recvOver->GetSession(), *connectPacket);

		// 정상적인 Recv진행
		gServerCore->RecvCall(recvOver->GetSession());
	}

	// 정상적인 접속이 아니면
	else {
		// 해당 Session의 Socket 및 recvOver 삭제
		closesocket(recvOver->GetSession()->GetSocket());
		delete recvOver;
		std::cout << "Connect Error" << std::endl;
		return;
	}
}

void ServerCore::RecvCallback(DWORD error, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag)
{
	ExpOver* recvOver = reinterpret_cast<ExpOver*>(pOver);
	Packet* recvPacket = reinterpret_cast<Packet*>(recvOver->GetBuffer());

	if (0 != error or 0 == numBytes) {
		closesocket(recvOver->GetSession()->GetSocket());
		delete recvOver;
		return;
	}

	// 받은 패킷의 종류에 따라 로직 수행 후 결과 Send (Connect 제외)
	recvOver->GetSession()->PacketProcessing(recvOver->GetSession(), *recvPacket);

	gServerCore->RecvCall(recvOver->GetSession());
}

void ServerCore::SendCallback(DWORD error, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag)
{
	ExpOver* sendOver = reinterpret_cast<ExpOver*>(pOver);

	if (0 != error) {
		closesocket(sendOver->GetSession()->GetSocket());
		delete sendOver;
		return;
	}
}

void ServerCore::errorDisplay(const char* msg, int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);

	std::cout << msg;
	std::wcout << L" 에러 " << lpMsgBuf << std::endl;

	while (true); // 디버깅 용

	LocalFree(lpMsgBuf);
}
