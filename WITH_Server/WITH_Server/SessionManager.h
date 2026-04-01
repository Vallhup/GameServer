#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include "Session.h"

class Connection;

class SessionManager final {
public:
	SessionManager() = default;
	~SessionManager() = default;

	SessionManager(const SessionManager&) = delete;
	SessionManager& operator=(const SessionManager&) = delete;
	SessionManager(SessionManager&&) = delete;
	SessionManager& operator=(SessionManager&&) = delete;

public:
	Session* CreateSession(const std::shared_ptr<Connection>& connection);

	Session* FindSession(SessionId sessionId) noexcept;
	const Session* FindSession(SessionId sessionId) const noexcept;

	bool BeginClose(SessionId sessionId, SessionCloseReason reason) noexcept;
	bool MarkClosed(SessionId sessionId, SessionCloseReason reason) noexcept;
	bool RemoveSession(SessionId sessionId) noexcept;

	void RemoveClosedSessions() noexcept;

	uint32_t GetSessionCount() const noexcept { return static_cast<uint32_t>(_sessions.size()); }
	bool IsEmpty() const noexcept { return _sessions.empty(); }

	void FillSessionIds(std::vector<SessionId>& outSessionIds) const;
	std::span<const std::unique_ptr<Session>> GetSessions() const noexcept;

private:
	SessionId AllocateSessionId() noexcept;
	size_t* FindIndex(SessionId sessionId) noexcept;
	const size_t* FindIndex(SessionId sessionId) const noexcept;

	bool RemoveAt(size_t index) noexcept;

private:
	SessionId _nextSessionId{ 1 };
	std::vector<std::unique_ptr<Session>> _sessions;
	std::unordered_map<SessionId, size_t> _sessionIndex;
};
