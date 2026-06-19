#include "pch.h"
#include "SessionManager.h"

#include "IocpConnection.h"

Session* SessionManager::CreateSession(IocpConnection* connection)
{
	if (connection == nullptr)
	{
		return nullptr;
	}

	// Use the session ID assigned by IocpNetworkBackend at accept time
	const SessionId sessionId = connection->GetSessionId();

	std::unique_ptr<Session> session =
		std::make_unique<Session>(sessionId, connection);

	Session* const sessionPtr = session.get();

	std::unique_lock lock(_mutex);
	_sessionIndex.try_emplace(sessionId, _sessions.size());
	_sessions.push_back(std::move(session));
	return sessionPtr;
}

Session* SessionManager::FindSession(SessionId sessionId) noexcept
{
	std::shared_lock lock(_mutex);
	const size_t* const index = FindIndex(sessionId);
	if (index == nullptr)
	{
		return nullptr;
	}

	return _sessions[*index].get();
}

const Session* SessionManager::FindSession(SessionId sessionId) const noexcept
{
	std::shared_lock lock(_mutex);
	const size_t* const index = FindIndex(sessionId);
	if (index == nullptr)
	{
		return nullptr;
	}

	return _sessions[*index].get();
}

bool SessionManager::MarkClosed(
	SessionId sessionId,
	SessionCloseReason reason) noexcept
{
	std::unique_lock lock(_mutex);
	const size_t* const index = FindIndex(sessionId);
	Session* const session =
		index != nullptr ? _sessions[*index].get() : nullptr;
	if (session == nullptr)
	{
		return false;
	}

	session->MarkClosed(reason);
	return true;
}

bool SessionManager::RemoveSession(SessionId sessionId) noexcept
{
	std::unique_lock lock(_mutex);
	const size_t* const index = FindIndex(sessionId);
	if (index == nullptr)
	{
		return false;
	}

	return RemoveAt(*index);
}

void SessionManager::RemoveClosedSessions() noexcept
{
	std::unique_lock lock(_mutex);
	size_t index = 0;
	while (index < _sessions.size())
	{
		Session* const session = _sessions[index].get();
		if (session != nullptr && session->IsClosed())
		{
			(void)RemoveAt(index);
			continue;
		}

		++index;
	}
}

void SessionManager::FillSessionIds(std::vector<SessionId>& outSessionIds) const
{
	std::shared_lock lock(_mutex);
	outSessionIds.clear();
	outSessionIds.reserve(_sessions.size());

	for (const std::unique_ptr<Session>& session : _sessions)
	{
		if (session == nullptr || session->IsClosed())
		{
			continue;
		}

		outSessionIds.push_back(session->GetSessionId());
	}
}

uint32_t SessionManager::GetSessionCount() const noexcept
{
	std::shared_lock lock(_mutex);
	return static_cast<uint32_t>(_sessions.size());
}

bool SessionManager::IsEmpty() const noexcept
{
	std::shared_lock lock(_mutex);
	return _sessions.empty();
}

std::span<const std::unique_ptr<Session>> SessionManager::GetSessions() const noexcept
{
	return std::span<const std::unique_ptr<Session>>{ _sessions };
}

SessionId SessionManager::AllocateSessionId() noexcept
{
	return _nextSessionId++;
}

size_t* SessionManager::FindIndex(SessionId sessionId) noexcept
{
	const auto it = _sessionIndex.find(sessionId);
	if (it == _sessionIndex.end())
	{
		return nullptr;
	}

	return &it->second;
}

const size_t* SessionManager::FindIndex(SessionId sessionId) const noexcept
{
	const auto it = _sessionIndex.find(sessionId);
	if (it == _sessionIndex.end())
	{
		return nullptr;
	}

	return &it->second;
}

bool SessionManager::RemoveAt(size_t index) noexcept
{
	if (index >= _sessions.size())
	{
		return false;
	}

	const size_t lastIndex = _sessions.size() - 1;
	const SessionId removedSessionId = _sessions[index]->GetSessionId();

	if (index != lastIndex)
	{
		std::swap(_sessions[index], _sessions[lastIndex]);

		Session* const movedSession = _sessions[index].get();
		if (movedSession != nullptr)
		{
			_sessionIndex[movedSession->GetSessionId()] = index;
		}
	}

	_sessions.pop_back();
	_sessionIndex.erase(removedSessionId);
	return true;
}
