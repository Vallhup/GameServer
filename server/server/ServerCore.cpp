#include <iostream>

#include "ServerCore.h"

ServerCore* gServerCore = new ServerCore;

LPFN_ACCEPTEX ServerCore::AcceptEx{ nullptr };

bool ServerCore::BindAcceptEx(SOCKET socket, GUID guid, LPVOID* fn)
{
	DWORD bytes = 0;
	return SOCKET_ERROR != WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid, sizeof(guid), fn, sizeof(*fn), OUT & bytes, NULL, NULL);
}

bool ServerCore::Init(short serverPort)
{
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData)) {
		std::cout << "WSAStartup Error : " << WSAGetLastError();
		return false;
	}

	SOCKET dummySocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (not BindAcceptEx(dummySocket, WSAID_ACCEPTEX, reinterpret_cast<LPVOID*>(&AcceptEx))) {
		errorDisplay("BindAcceptEx Error : ", WSAGetLastError());
		return false;
	}

	_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == _listenSocket) {
		std::cout << "WSASocket Error";
		return false;
	}

	int optval = 1;

	if (SOCKET_ERROR == setsockopt(_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval))) {
		std::cout << "reuse error";
		return false;
	}

	linger lingerOption;
	lingerOption.l_linger = 0;
	lingerOption.l_onoff = 0;

	if (SOCKET_ERROR == setsockopt(_listenSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(lingerOption))) {
		std::cout << "linger error";
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
	//Init(SERVER_PORT);

	while (true) {
		ClientAccept();
	}

	WSACleanup();
}

void ServerCore::ClientAccept()
{
	SOCKADDR_IN clientAddr;
	INT addrSize = sizeof(SOCKADDR_IN);

	SOCKET clientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == clientSocket) {
		return;
	}

	char buffer[1024] = {};
	ExpOver acceptOver;
	DWORD recvBytes = 0;

	if (not AcceptEx(_listenSocket, clientSocket, acceptOver.GetBuffer(), 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		OUT & recvBytes, acceptOver.GetOverPtr()))
	{
		int error = WSAGetLastError();
		if (WSA_IO_PENDING != error) {
			errorDisplay("WSAAccept : ", error);
			return;
		}
	}

	DWORD flags = 0;

	if (SOCKET_ERROR == WSAGetOverlappedResult(_listenSocket, acceptOver.GetOverPtr(), &recvBytes, TRUE, &flags)) {
		std::cerr << "WSAGetOverlappedResult failed" << std::endl;
		closesocket(_listenSocket);
		WSACleanup();
		return;
	}

	AcceptCallback(clientSocket);
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

void ServerCore::AcceptCallback(SOCKET clientSocket)
{
	std::unique_ptr<Session> session = std::make_unique<Session>(gServerCore->_sessionId++, clientSocket);

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

	gServerCore->_sessions[session->GetSessionId()] = std::move(session);

	gServerCore->ConnectRecvCall(sessionPtr->GetSocket());
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
