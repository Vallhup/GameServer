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

	const SessionBinding* existingSessionBinding = FindBySession(sessionId);
	if (existingSessionBinding != nullptr)
	{
		_sessionByNetId.erase(existingSessionBinding->controlledNetId);
	}

	const SessionBinding* existingNetBinding = FindByNetId(controlledNetId);
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
	const auto it = _bindingBySession.find(sessionId);
	if (it == _bindingBySession.end())
	{
		return nullptr;
	}

	return &it->second;
}

const SessionBinding* SessionBindingRegistry::FindByNetId(NetId controlledNetId) const
{
	const auto it = _sessionByNetId.find(controlledNetId);
	if (it == _sessionByNetId.end())
	{
		return nullptr;
	}

	return FindBySession(it->second);
}

NetId SessionBindingRegistry::FindControlledNetId(SessionId sessionId) const
{
	const SessionBinding* binding = FindBySession(sessionId);
	if (binding == nullptr)
	{
		return NetId::Invalid();
	}

	return binding->controlledNetId;
}

SessionId SessionBindingRegistry::FindOwnerSession(NetId controlledNetId) const
{
	const auto it = _sessionByNetId.find(controlledNetId);
	if (it == _sessionByNetId.end())
	{
		return 0;
	}

	return it->second;
}

WorldId SessionBindingRegistry::FindCurrentWorldId(SessionId sessionId) const
{
	const SessionBinding* binding = FindBySession(sessionId);
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
	const SessionBinding* binding = FindBySession(sessionId);
	return binding != nullptr && binding->IsValid();
}

void SessionBindingRegistry::Clear()
{
	_bindingBySession.clear();
	_sessionByNetId.clear();
}
