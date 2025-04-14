#include "pch.h"
#include "CallbackServerCore.h"

const int MAX_CLIENT = 10;

ServerCore gServerCore;
std::unordered_map<int, std::shared_ptr<Session>> ServerCore::_sessions;

bool ServerCore::Start(short serverPort)
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
		if (_sessions.size() < MAX_CLIENT) {
			ClientAccept();
		}

		SleepEx(0, TRUE);
	}
}

void ServerCore::End()
{
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

	std::shared_ptr<Session> session = std::make_shared<Session>(_sessionId, clientSocket);
	_sessions.try_emplace(_sessionId, session);
	_sessions[_sessionId]->doRecv();

	_sessionId++;
}

void errorDisplay(const char* msg, int err_no)
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

void gRecvCallback(DWORD err, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag)
{
	// 어떤 Session의 Recv인지 알아서 그 Session의 RecvCallback 호출
	RecvOver* recvOver = reinterpret_cast<RecvOver*>(pOver);
	int myId = recvOver->_owner->GetSessionId();

	if (numBytes == 0) {
		if (ServerCore::_sessions.count(myId) != 0) {
			ServerCore::_sessions.erase(myId);
			return;
		}
	}

	gServerCore._sessions[myId]->RecvCallback(numBytes);
}

void gSendCallback(DWORD err, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag)
{
	// 어떤 Session의 Send인지 알아서 그 Session의 SendCallback 호출
	SendOver* sendOver = reinterpret_cast<SendOver*>(pOver);
	int myId = sendOver->_owner->GetSessionId();

	if (numBytes == 0) {
		if (ServerCore::_sessions.count(myId) != 0) {
			ServerCore::_sessions.erase(myId);
			return;
		}
	}

	gServerCore._sessions[myId]->SendCallback();
}