#include "pch.h"
#include "Listener.h"

Listener::~Listener()
{
	closesocket(_socket);

	for (AcceptOver* acceptOver : _acceptOvers) {
		delete acceptOver;
	}
}

bool Listener::StartAccept(std::shared_ptr<ServerService> service)
{
	// TODO : 기존 Overlapped IO Callback에서 Accept하던것과 유사한 동작
	return true;
}

void Listener::CloseSocket()
{
	closesocket(_socket);
}

HANDLE Listener::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Listener::Dispatch(ExpOver* expOver, int numOfBytes)
{
	if (expOver->_operationType == OperationType::Accept) {
		AcceptOver* acceptOver = static_cast<AcceptOver*>(expOver);
		AcceptCallback(acceptOver);
	}
}

void Listener::doAccept(AcceptOver* acceptOver)
{
	// TODO : 비동기 Accept
}

void Listener::AcceptCallback(AcceptOver* acceptOver)
{
	// TODO : 새로운 Session 만들고 다시 doAccept
}
