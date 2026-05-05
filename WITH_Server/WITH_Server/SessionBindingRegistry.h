#pragma once

#include <shared_mutex>
#include <vector>
#include <unordered_map>

#include "NetId.h"
#include "Session.h"
#include "WorldId.h"

struct SessionBinding
{
	SessionId sessionId{ 0 };
	NetId controlledNetId{ NetId::Invalid() };
	WorldId currentWorldId{ WorldId::Invalid() };

	bool IsValid() const noexcept
	{
		return sessionId != 0 && controlledNetId.IsValid();
	}
};

class SessionBindingRegistry final {
public:
	bool Bind(SessionId sessionId, NetId controlledNetId, WorldId currentWorldId);

	bool Unbind(SessionId sessionId);
	bool UnbindByNetId(NetId controlledNetId);

	const SessionBinding* FindBySession(SessionId sessionId) const;
	const SessionBinding* FindByNetId(NetId controlledNetId) const;

	NetId FindControlledNetId(SessionId sessionId) const;
	SessionId FindOwnerSession(NetId controlledNetId) const;
	WorldId FindCurrentWorldId(SessionId sessionId) const;
	void CollectSessionsInWorld(
		WorldId worldId,
		std::vector<SessionId>& outSessionIds) const;

	bool UpdateWorld(SessionId sessionId, WorldId currentWorldId);

	bool HasBinding(SessionId sessionId) const;
	void Clear();

private:
	const SessionBinding* FindBySessionNoLock(SessionId sessionId) const noexcept;
	const SessionBinding* FindByNetIdNoLock(NetId controlledNetId) const noexcept;

	mutable std::shared_mutex _mutex;
	std::unordered_map<SessionId, SessionBinding> _bindingBySession;
	std::unordered_map<NetId, SessionId> _sessionByNetId;
};
