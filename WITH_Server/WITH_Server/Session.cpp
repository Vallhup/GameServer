#include "pch.h"
#include "Session.h"

#include "IocpConnection.h"
#include "SendBuffer.h"

Session::Session(SessionId sessionId, IocpConnection* connection)
	: _sessionId(sessionId)
	, _connection(connection)
{
}

bool Session::Send(SendBuffer* buffer) noexcept
{
	if (buffer == nullptr || _connection == nullptr)
	{
		return false;
	}

	return _connection->RegisterSend(buffer);
}

void Session::MarkClosed(SessionCloseReason reason) noexcept
{
	_isClosed = true;
	_closeReason = reason;
	_connection = nullptr;
}
