#pragma once

#include <cstdint>
#include <memory>

#include "NetId.h"

class Connection;
struct SendBuffer;

using SessionId = uint32_t;

enum class SessionState : uint8_t
{
	Connected,
	Authenticated,
	InGame,
	Closing,
	Closed,
};

enum class SessionCloseReason : uint8_t
{
	None,
	RemoteClosed,
	LocalRequested,
	ProtocolError,
	Timeout,
	ServerShutdown,
};

class Session final {
public:
	Session(SessionId sessionId, const std::shared_ptr<Connection>& connection);
	~Session() = default;

	Session(const Session&) = delete;
	Session& operator=(const Session&) = delete;
	Session(Session&&) = delete;
	Session& operator=(Session&&) = delete;

public:
	SessionId GetSessionId() const noexcept { return _sessionId; }
	SessionState GetState() const noexcept { return _state; }
	SessionCloseReason GetCloseReason() const noexcept { return _closeReason; }

	bool IsConnected() const noexcept { return _state == SessionState::Connected; }
	bool IsAuthenticated() const noexcept { return _state == SessionState::Authenticated; }
	bool IsInGame() const noexcept { return _state == SessionState::InGame; }
	bool IsClosing() const noexcept { return _state == SessionState::Closing; }
	bool IsClosed() const noexcept { return _state == SessionState::Closed; }

	NetId GetPlayerNetId() const noexcept { return _playerNetId; }
	bool HasBoundPlayer() const noexcept { return _playerNetId.IsValid(); }

	bool Send(SendBuffer* buffer) noexcept;

	bool CompleteLogin() noexcept;
	bool EnterInGame(NetId playerNetId) noexcept;
	bool LeaveGame() noexcept;
	bool BeginClose(SessionCloseReason reason) noexcept;
	void MarkClosed(SessionCloseReason reason) noexcept;

private:
	bool CanSendState() const noexcept;

	bool BindPlayer(NetId playerNetId) noexcept;
	void ClearPlayer() noexcept { _playerNetId = NetId::Invalid(); }

	SessionId _sessionId{ 0 };
	SessionState _state{ SessionState::Connected };
	SessionCloseReason _closeReason{ SessionCloseReason::None };
	NetId _playerNetId{ NetId::Invalid() };
	std::shared_ptr<Connection> _connection;
};
