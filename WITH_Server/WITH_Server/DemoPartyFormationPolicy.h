#pragma once

#include <vector>

#include "PartyService.h"

class IDemoPartyTransitionQuery
{
public:
	virtual ~IDemoPartyTransitionQuery() = default;
	virtual bool IsClientTransitionPending(SessionId sessionId) const = 0;
};

class DemoPartyFormationPolicy final
{
public:
	DemoPartyFormationPolicy(
		PartyService& partyService,
		IPartySessionQuery& sessionQuery,
		IDemoPartyTransitionQuery& transitionQuery);

	PartyResult EnsurePartyForWorldTransition(
		SessionId actorSessionId,
		WorldId sourceWorldId,
		double nowSec);

private:
	PartyService& _partyService;
	IPartySessionQuery& _sessionQuery;
	IDemoPartyTransitionQuery& _transitionQuery;
	std::vector<SessionId> _scratchSessions;
};
