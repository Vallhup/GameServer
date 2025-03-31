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
	ServerCore() : _listenSocket(INVALID_SOCKET), _sessionId(0), _totalReceived(0), _packetSize(0)
	{
	}
	
	bool Init(short serverPort);
	void MainLoop();
	void ClientAccept();

	void RecvCall(Session* session) const;
	void SendCall(Session* session, Packet* packet) const;

	static void CALLBACK RecvCallback(DWORD error, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag);
	static void CALLBACK SendCallback(DWORD error, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag);

	static void CALLBACK AcceptCallback(SOCKET clientSocket);

	static void errorDisplay(const char* msg, int err_no);

private:
	SOCKET _listenSocket;
	std::unordered_map<int, std::unique_ptr<Session>> _sessions;
	std::vector<Packet> _recvBuffer = {};
	size_t _totalReceived;
	size_t _packetSize;
	int _sessionId;

};

extern ServerCore* gServerCore;