#include "pch.h"
#include "Session.h"

#include "IocpConnection.h"
#include "SendBuffer.h"

Session::Session(SessionId sessionId, IocpConnection* connection)
	: _sessionId(sessionId)
	, _connection(connection)
{
}

bool Session::CanSendState() const noexcept
{
	return IsConnected() || IsAuthenticated() || IsInGame();
}

bool Session::BindPlayer(NetId playerNetId) noexcept
{
	if (!playerNetId.IsValid() || HasBoundPlayer())
	{
		return false;
	}

	_playerNetId = playerNetId;
	return true;
}

bool Session::Send(SendBuffer* buffer) noexcept
{
	if (!CanSendState() || buffer == nullptr || _connection == nullptr)
	{
		return false;
	}

	return _connection->RegisterSend(buffer);
}

bool Session::CompleteLogin() noexcept
{
	if (_state != SessionState::Connected)
	{
		return false;
	}

	_state = SessionState::Authenticated;
	return true;
}

bool Session::EnterInGame(NetId playerNetId) noexcept
{
	if (_state != SessionState::Authenticated)
	{
		return false;
	}

	if (BindPlayer(playerNetId))
	{
		_state = SessionState::InGame;
		return true;
	}

	return false;
}

bool Session::LeaveGame() noexcept
{
	if (_state != SessionState::InGame)
	{
		return false;
	}

	_state = SessionState::Authenticated;
	ClearPlayer();
	return true;
}

bool Session::BeginClose(SessionCloseReason reason) noexcept
{
	if (_state != SessionState::Connected &&
		_state != SessionState::Authenticated &&
		_state != SessionState::InGame)
	{
		return false;
	}

	_state = SessionState::Closing;
	_closeReason = reason;

	if (_connection != nullptr)
	{
		_connection->Close();
		_connection = nullptr;
	}

	return true;
}

void Session::MarkClosed(SessionCloseReason reason) noexcept
{
	_state = SessionState::Closed;
	_closeReason = reason;
	_connection = nullptr;
	ClearPlayer();
}
