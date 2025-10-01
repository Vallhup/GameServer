#pragma once

#include <winsock2.h>
#include <array>
#include <vector>

#include "RecvBuffer.h"

enum class OperationType : char
{
	Recv,
	Send
};

class IocpObject;

class ExpOver : public OVERLAPPED {
public:
	ExpOver() = delete;
	ExpOver(OperationType opType);
	virtual ~ExpOver() = default;

public:
	void Init();
	bool CheckOpType(OperationType opType);

protected:
	OperationType _opType;
};

class RecvOver : public ExpOver {
public:
	RecvOver();
	virtual ~RecvOver() = default;

public:
	int SetBuffers();

public:
	RecvBuffer	  _buffer;
	std::array<WSABUF, 2> _wsaBuf;
};

class SendOver : public ExpOver {
public:
	SendOver();
	virtual ~SendOver() = default;

public:
	void SetBuffers(const std::vector<std::vector<char>>&& buffers);

public:
	std::vector<WSABUF> _wsaBufs;
	std::vector<std::vector<char>> _sendDataList;
};