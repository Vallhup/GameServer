#pragma once

#include <cstdint>

#include "NetworkSessionTypes.h"

class IocpConnection;
struct SendBuffer;

class Session final {
public:
	Session(SessionId sessionId, IocpConnection* connection);
	~Session() = default;

	Session(const Session&) = delete;
	Session& operator=(const Session&) = delete;
	Session(Session&&) = delete;
	Session& operator=(Session&&) = delete;

public:
	SessionId          GetSessionId()   const noexcept { return _sessionId; }
	SessionCloseReason GetCloseReason() const noexcept { return _closeReason; }
	bool               IsClosed()       const noexcept { return _isClosed; }

	void SetConnection(IocpConnection* conn) noexcept { _connection = conn; }
	bool HasConnection()                    const noexcept { return _connection != nullptr; }

	bool Send(SendBuffer* buffer) noexcept;
	void MarkClosed(SessionCloseReason reason) noexcept;

private:
	SessionId          _sessionId{ 0 };
	SessionCloseReason _closeReason{ SessionCloseReason::None };
	bool               _isClosed{ false };
	IocpConnection*    _connection{ nullptr };
};
