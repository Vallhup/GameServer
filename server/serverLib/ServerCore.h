#pragma once

#include "pch.h"

extern const short SERVER_PORT;

class ServerCore
{
public:
	static LPFN_ACCEPTEX AcceptEx;

public:
	bool BindAcceptEx(SOCKET socket, GUID guid, LPVOID* fn);

public:
	ServerCore() : _listenSocket(INVALID_SOCKET), _sessionId(0)
	{
	}
	
	bool Init(short serverPort);
	void MainLoop();
	void ClientAccept();

	void ConnectRecvCall(SOCKET clientSocket);

	void RecvCall(Session* session) const;
	void SendCall(Session* session, const Packet& packet) const;

	static void CALLBACK ConnectRecvCallback(DWORD error, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag);

	static void CALLBACK RecvCallback(DWORD error, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag);
	static void CALLBACK SendCallback(DWORD error, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag);

	static void CALLBACK AcceptCallback(SOCKET clientSocket);

	static void errorDisplay(const char* msg, int err_no);

private:
	SOCKET _listenSocket;
	std::unordered_map<int, std::unique_ptr<Session>> _sessions;
	int _sessionId;

};

extern ServerCore* gServerCore;