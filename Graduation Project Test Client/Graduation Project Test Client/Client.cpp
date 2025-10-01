#include "Client.h"

bool Client::Connect()
{
	return false;
}

void Client::Disconnect()
{
}

HANDLE Client::GetHandle() const
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Client::Dispatch(ExpOver* expOver, int numOfBytes)
{
}
