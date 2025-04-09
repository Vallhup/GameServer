#pragma once

extern const int MAX_CLIENT;

class Session;

class ServerCore
{
public:
	ServerCore() : _listenSocket(INVALID_SOCKET), _sessionId(0)
	{
	}

	bool Start(short serverPort);
	void MainLoop();
	void End();
	void ClientAccept();

public:
	std::unordered_map<int, Session> _sessions;

private:
	SOCKET _listenSocket;
	int _sessionId;
};

extern ServerCore gServerCore;

void errorDisplay(const char* msg, int err_no);

void CALLBACK gRecvCallback(DWORD err, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag);
void CALLBACK gSendCallback(DWORD err, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag);