#include "pch.h"
#include "SessionBindingRegistry.h"

#include <algorithm>

bool SessionBindingRegistry::Bind(
	SessionId sessionId,
	NetId controlledNetId,
	WorldId currentWorldId)
{
	if (sessionId == 0 || !controlledNetId.IsValid())
	{
		return false;
	}

	std::unique_lock lock{ _mutex };

	const SessionBinding* existingSessionBinding = FindBySessionNoLock(sessionId);
	if (existingSessionBinding != nullptr)
	{
		_sessionByNetId.erase(existingSessionBinding->controlledNetId);
	}

	const SessionBinding* existingNetBinding = FindByNetIdNoLock(controlledNetId);
	if (existingNetBinding != nullptr && existingNetBinding->sessionId != sessionId)
	{
		_bindingBySession.erase(existingNetBinding->sessionId);
	}

	_bindingBySession[sessionId] = SessionBinding{
		sessionId,
		controlledNetId,
		currentWorldId
	};
	_sessionByNetId[controlledNetId] = sessionId;
	return true;
}

bool SessionBindingRegistry::Unbind(SessionId sessionId)
{
	std::unique_lock lock{ _mutex };

	auto it = _bindingBySession.find(sessionId);
	if (it == _bindingBySession.end())
	{
		return false;
	}

	_sessionByNetId.erase(it->second.controlledNetId);
	_bindingBySession.erase(it);
	return true;
}

bool SessionBindingRegistry::UnbindByNetId(NetId controlledNetId)
{
	std::unique_lock lock{ _mutex };

	auto it = _sessionByNetId.find(controlledNetId);
	if (it == _sessionByNetId.end())
	{
		return false;
	}

	_bindingBySession.erase(it->second);
	_sessionByNetId.erase(it);
	return true;
}

const SessionBinding* SessionBindingRegistry::FindBySession(SessionId sessionId) const
{
	std::shared_lock lock{ _mutex };
	return FindBySessionNoLock(sessionId);
}

const SessionBinding* SessionBindingRegistry::FindByNetId(NetId controlledNetId) const
{
	std::shared_lock lock{ _mutex };
	return FindByNetIdNoLock(controlledNetId);
}

NetId SessionBindingRegistry::FindControlledNetId(SessionId sessionId) const
{
	std::shared_lock lock{ _mutex };
	const SessionBinding* binding = FindBySessionNoLock(sessionId);
	if (binding == nullptr)
	{
		return NetId::Invalid();
	}

	return binding->controlledNetId;
}

SessionId SessionBindingRegistry::FindOwnerSession(NetId controlledNetId) const
{
	std::shared_lock lock{ _mutex };
	const auto it = _sessionByNetId.find(controlledNetId);
	if (it == _sessionByNetId.end())
	{
		return 0;
	}

	return it->second;
}

WorldId SessionBindingRegistry::FindCurrentWorldId(SessionId sessionId) const
{
	std::shared_lock lock{ _mutex };
	const SessionBinding* binding = FindBySessionNoLock(sessionId);
	if (binding == nullptr)
	{
		return WorldId::Invalid();
	}

	return binding->currentWorldId;
}

void SessionBindingRegistry::CollectSessionsInWorld(
	WorldId worldId,
	std::vector<SessionId>& outSessionIds) const
{
	outSessionIds.clear();
	if (!worldId.IsValid())
	{
		return;
	}

	std::shared_lock lock{ _mutex };
	for (const auto& [sessionId, binding] : _bindingBySession)
	{
		if (binding.currentWorldId == worldId && binding.IsValid())
		{
			outSessionIds.push_back(sessionId);
		}
	}

	std::sort(outSessionIds.begin(), outSessionIds.end());
}

bool SessionBindingRegistry::UpdateWorld(SessionId sessionId, WorldId currentWorldId)
{
	std::unique_lock lock{ _mutex };

	auto it = _bindingBySession.find(sessionId);
	if (it == _bindingBySession.end())
	{
		return false;
	}

	it->second.currentWorldId = currentWorldId;
	return true;
}

bool SessionBindingRegistry::HasBinding(SessionId sessionId) const
{
	std::shared_lock lock{ _mutex };
	const SessionBinding* binding = FindBySessionNoLock(sessionId);
	return binding != nullptr && binding->IsValid();
}

void SessionBindingRegistry::Clear()
{
	std::unique_lock lock{ _mutex };
	_bindingBySession.clear();
	_sessionByNetId.clear();
}

const SessionBinding* SessionBindingRegistry::FindBySessionNoLock(SessionId sessionId) const noexcept
{
	const auto it = _bindingBySession.find(sessionId);
	if (it == _bindingBySession.end())
	{
		return nullptr;
	}

	return &it->second;
}

const SessionBinding* SessionBindingRegistry::FindByNetIdNoLock(NetId controlledNetId) const noexcept
{
	const auto it = _sessionByNetId.find(controlledNetId);
	if (it == _sessionByNetId.end())
	{
		return nullptr;
	}

	return FindBySessionNoLock(it->second);
}
