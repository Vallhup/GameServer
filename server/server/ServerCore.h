#pragma once

#include <WinSock2.h>
#include <unordered_map>
#include <memory>
#include "Session.h"
#include "ExpOver.h"

class ServerCore
{
public:
	ServerCore() : _listenSocket(INVALID_SOCKET), _sessionId(0)
	{
	}
	
	bool Init(short serverPort);
	void MainLoop();
	void ClientAccept();

	void RecvCall(Session* session) const;
	void SendCall(Session* session, const Packet& packet) const;

	static void CALLBACK RecvCallback(DWORD error, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag);
	static void CALLBACK SendCallback(DWORD error, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag);

	static void errorDisplay(const char* msg, int err_no);

private:
	SOCKET _listenSocket;
	std::unordered_map<int, std::unique_ptr<Session>> _sessions;
	int _sessionId;

};

extern ServerCore* gServerCore;