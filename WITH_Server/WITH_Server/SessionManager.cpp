#include "pch.h"
#include "SessionManager.h"

#include "Connection.h"

Session* SessionManager::CreateSession(
	const std::shared_ptr<Connection>& connection)
{
	if (connection == nullptr)
	{
		return nullptr;
	}

	const SessionId sessionId = AllocateSessionId();
	connection->SetId(sessionId);

	std::unique_ptr<Session> session =
		std::make_unique<Session>(sessionId, connection);

	Session* const sessionPtr = session.get();
	_sessionIndex.try_emplace(sessionId, _sessions.size());
	_sessions.push_back(std::move(session));
	return sessionPtr;
}

Session* SessionManager::FindSession(SessionId sessionId) noexcept
{
	const size_t* const index = FindIndex(sessionId);
	if (index == nullptr)
	{
		return nullptr;
	}

	return _sessions[*index].get();
}

const Session* SessionManager::FindSession(SessionId sessionId) const noexcept
{
	const size_t* const index = FindIndex(sessionId);
	if (index == nullptr)
	{
		return nullptr;
	}

	return _sessions[*index].get();
}

bool SessionManager::BeginClose(
	SessionId sessionId,
	SessionCloseReason reason) noexcept
{
	Session* const session = FindSession(sessionId);
	if (session == nullptr)
	{
		return false;
	}

	return session->BeginClose(reason);
}

bool SessionManager::MarkClosed(
	SessionId sessionId,
	SessionCloseReason reason) noexcept
{
	Session* const session = FindSession(sessionId);
	if (session == nullptr)
	{
		return false;
	}

	session->MarkClosed(reason);
	return true;
}

bool SessionManager::RemoveSession(SessionId sessionId) noexcept
{
	const size_t* const index = FindIndex(sessionId);
	if (index == nullptr)
	{
		return false;
	}

	return RemoveAt(*index);
}

void SessionManager::RemoveClosedSessions() noexcept
{
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
	outSessionIds.clear();
	outSessionIds.reserve(_sessions.size());

	for (const std::unique_ptr<Session>& session : _sessions)
	{
		if (session == nullptr)
		{
			continue;
		}

		outSessionIds.push_back(session->GetSessionId());
	}
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
