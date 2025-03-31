#include "pch.h"

ServerCore* gServerCore = new ServerCore;
const short SERVER_PORT = 7777;

LPFN_ACCEPTEX ServerCore::AcceptEx{ nullptr };

bool ServerCore::BindAcceptEx(SOCKET socket, GUID guid, LPVOID* fn)
{
	DWORD bytes = 0;
	if (SOCKET_ERROR == WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid, sizeof(guid), fn, sizeof(*fn), OUT & bytes, NULL, NULL)) {
		std::cout << "AcceptEx 함수 포인터 바인딩 실패." << std::endl;
		return false;
	}

	return true;
}

bool ServerCore::Init(short serverPort)
{
	std::wcout.imbue(std::locale("korean"));

	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData)) {
		std::cout << "WSAStartup Error : " << WSAGetLastError();
		WSACleanup();
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

	if (SOCKET_ERROR == listen(_listenSocket, 10)) {
		std::cout << "listen Error";
		return false;
	}

	return true;
}

void ServerCore::MainLoop()
{
	while (true) {
		std::cout << "Before ClientAccept" << std::endl;
		ClientAccept();
		std::cout << "Before SleepEx" << std::endl;
		SleepEx(0, TRUE);
		std::cout << "After SleepEx" << std::endl;
	}
	
	WSACleanup();
}

void ServerCore::ClientAccept()
{
	SOCKADDR_IN clientAddr;
	INT addrSize = sizeof(SOCKADDR_IN);

	SOCKET clientSocket = accept(_listenSocket, (sockaddr*)&clientAddr, &addrSize);
	if (INVALID_SOCKET == clientSocket) {
		errorDisplay("accept Error: ", WSAGetLastError());
		return;
	}

	std::cout << "Client accepted, setting up session" << std::endl;
	
	AcceptCallback(clientSocket);
}

void ServerCore::RecvCall(Session* session) const
{
	std::cout << "Recv" << std::endl;

	ExpOver* recvOver = new ExpOver(session);
	DWORD recvFlag = 0;

	if (session->GetSocket() != INVALID_SOCKET) {
		auto ret = WSARecv(session->GetSocket(), recvOver->GetWsabuf(), 1, NULL, &recvFlag, recvOver->GetOverPtr(), RecvCallback);
		if (SOCKET_ERROR == ret) {
			auto error = WSAGetLastError();
			if (WSA_IO_PENDING != error) {
				errorDisplay("WSARecv Error : ", error);
				std::cout << _listenSocket << " " << session->GetSocket() << std::endl;
				delete recvOver;
			}
			else {
				std::cout << "WSARecv started successfully." << std::endl;
			}
		}
	}
	
}

void ServerCore::SendCall(Session* session, Packet* packet) const
{
	std::cout << "Send" << std::endl;
	ExpOver* sendOver = new ExpOver(session, packet);
	DWORD sentBytes = 0;

	auto ret = WSASend(session->GetSocket(), sendOver->GetWsabuf(), 1, &sentBytes, 0, sendOver->GetOverPtr(), SendCallback);
	if (SOCKET_ERROR == ret) {
		auto error = WSAGetLastError();
		if (WSA_IO_PENDING != error) {
			errorDisplay("WSARecv : ", error);
			delete sendOver;
		}
	}

	std::cout << sentBytes << std::endl;
}

void ServerCore::RecvCallback(DWORD error, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag)
{
	std::cout << "RecvCallback" << std::endl;

	if (error != 0) {
		std::cout << "WSARecv Error: " << error << std::endl;
		closesocket(reinterpret_cast<ExpOver*>(pOver)->GetSession()->GetSocket());
		delete reinterpret_cast<ExpOver*>(pOver);
		return;
	}

	if (numBytes == 0) {
		std::cout << "Client disconnected" << std::endl;
		closesocket(reinterpret_cast<ExpOver*>(pOver)->GetSession()->GetSocket());
		delete reinterpret_cast<ExpOver*>(pOver);
		return;
	}

	ExpOver* recvOver = reinterpret_cast<ExpOver*>(pOver);
	Packet* recvPacket = reinterpret_cast<Packet*>(recvOver->GetBuffer());

	recvOver->GetSession()->PacketProcessing(recvOver->GetSession(), recvPacket);

	Session* session = recvOver->GetSession();
	
	gServerCore->RecvCall(session);

	delete recvOver;

}

void ServerCore::SendCallback(DWORD error, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag)
{
	std::cout << "SendCallback" << std::endl;
	ExpOver* sendOver = reinterpret_cast<ExpOver*>(pOver);
	
	if (0 != error) {
		closesocket(sendOver->GetSession()->GetSocket());
	}

	delete sendOver;
}

void ServerCore::AcceptCallback(SOCKET clientSocket)
{
	std::cout << "Accept" << std::endl;

	std::unique_ptr<Session> session = std::make_unique<Session>(gServerCore->_sessionId++, clientSocket);

	Session* sessionPtr = session.get();
	
	gServerCore->RecvCall(sessionPtr);

	gServerCore->_sessions[session->GetSessionId()] = std::move(session);
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

	while (true);

	LocalFree(lpMsgBuf);
}
