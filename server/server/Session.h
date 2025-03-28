#pragma once

#include <WS2tcpip.h>
#include "ExpOver.h"

#pragma comment (lib, "WS2_32.LIB")

// Client의 정보 (고유 id, 연결 socket 등)
// 컨텐츠와 관련된 작업들
class Session
{
public:
	Session() = delete;
	Session(int id, SOCKET s) : _id(id), _socket(s)
	{
	}

	~Session() { closesocket(_socket); }

	void recvCallback(DWORD err, DWORD numBytes, LPWSAOVERLAPPED pOver, DWORD flag)
	{

	}

private:


private:
	SOCKET	_socket;
	int		_id;
};


// 1. 새로운 Client 접속 -> Client에서 PACKET_CONNECT Server로 Send -> Server는 PACKET 검사해서 Connect 여부 Client로 Send -> Client는 받아서 초기화 후 Rendering
// 2. Client 이동 -> PACKET_MOVE Server로 Send -> Server는 받아서 
//
//