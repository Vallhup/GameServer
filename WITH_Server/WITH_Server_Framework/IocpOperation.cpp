#include "pch.h"
#include "IocpOperation.h"
#include "IocpConnection.h"
#include "IocpAcceptor.h"

void RecvOp::Init(IocpConnection* conn) noexcept
{
	std::memset(&ov, 0, sizeof(ov));
	owner = conn;
	dispatch = [](IocpOperation* base, DWORD bytes, bool success)
	{
		static_cast<RecvOp*>(base)->owner->OnRecvComplete(bytes, success);
	};
}

void AcceptOp::Init(IocpAcceptor* acceptor, SOCKET preparedSocket) noexcept
{
	std::memset(&ov, 0, sizeof(ov));
	owner = acceptor;
	socket = preparedSocket;
	dispatch = [](IocpOperation* base, DWORD bytes, bool success)
	{
		auto* self = static_cast<AcceptOp*>(base);
		self->owner->OnAcceptComplete(self, bytes, success);
	};
}

void SendOp::Init(IocpConnection* conn) noexcept
{
	std::memset(&ov, 0, sizeof(ov));
	owner = conn;
	dispatch = [](IocpOperation* base, DWORD bytes, bool success)
	{
		auto* self = static_cast<SendOp*>(base);
		self->owner->OnSendComplete(self, success);
	};
}

void SendOp::Reset() noexcept
{
	wsaBufs.clear();
	ownedBuffers.clear();
	owner = nullptr;
	std::memset(&ov, 0, sizeof(ov));
}
