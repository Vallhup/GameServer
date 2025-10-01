#pragma once

#include <winsock2.h>
#include <WS2tcpip.h>
#include <mswsock.h>
#include <Windows.h>

#include <atomic>

#include "IocpCore.h"
#include "vec3.h"

class Client : public IocpObject {
public:
	virtual ~Client() = default;

public:
	bool Connect();
	void Disconnect();

public:
	virtual HANDLE GetHandle() const override;
	virtual void Dispatch(class ExpOver* expOver, int numOfBytes = 0) override;

private:
	int _id;
	vec3 _pos;
	SOCKET _socket;

	std::atomic<bool> _connected;
};