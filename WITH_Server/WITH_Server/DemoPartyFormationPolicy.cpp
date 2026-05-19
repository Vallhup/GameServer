#include "pch.h"
#include "DemoPartyFormationPolicy.h"

#include <algorithm>

DemoPartyFormationPolicy::DemoPartyFormationPolicy(
	PartyService& partyService,
	IPartySessionQuery& sessionQuery,
	IDemoPartyTransitionQuery& transitionQuery)
	: _partyService(partyService)
	, _sessionQuery(sessionQuery)
	, _transitionQuery(transitionQuery)
{
}

PartyResult DemoPartyFormationPolicy::EnsurePartyForWorldTransition(
	SessionId actorSessionId,
	WorldId sourceWorldId,
	double nowSec)
{
	if (actorSessionId == 0 ||
		!sourceWorldId.IsValid() ||
		_transitionQuery.IsClientTransitionPending(actorSessionId) ||
		!_sessionQuery.IsPartyEligible(actorSessionId) ||
		_sessionQuery.FindCurrentWorldId(actorSessionId) != sourceWorldId)
	{
		return PartyResult{ .error = PartyError::InvalidSession };
	}

	const PartyId existingPartyId =
		_partyService.FindPartyBySession(actorSessionId);
	if (existingPartyId != 0)
	{
		const PartyRecord* const party =
			_partyService.FindParty(existingPartyId);
		if (party == nullptr)
		{
			return PartyResult{
				.error = PartyError::PartyNotFound,
				.partyId = existingPartyId
			};
		}

		if (party->lifecycle == PartyLifecycleState::WorldEntryPending)
		{
			return PartyResult{
				.error = PartyError::WorldEntryAlreadyPending,
				.partyId = existingPartyId
			};
		}

		return PartyResult{
			.error = PartyError::None,
			.partyId = existingPartyId
		};
	}

	_scratchSessions.clear();
	_sessionQuery.CollectSessionsInWorld(sourceWorldId, _scratchSessions);
	std::sort(_scratchSessions.begin(), _scratchSessions.end());

	std::vector<SessionId> selectedSessions;
	selectedSessions.reserve(MaxPartyMembers);

	if (std::find(
		_scratchSessions.begin(),
		_scratchSessions.end(),
		actorSessionId) == _scratchSessions.end())
	{
		return PartyResult{ .error = PartyError::InvalidSession };
	}

	selectedSessions.push_back(actorSessionId);
	for (SessionId candidateSessionId : _scratchSessions)
	{
		if (candidateSessionId == actorSessionId)
		{
			continue;
		}

		if (_transitionQuery.IsClientTransitionPending(candidateSessionId) ||
			!_sessionQuery.IsPartyEligible(candidateSessionId) ||
			_sessionQuery.FindCurrentWorldId(candidateSessionId) != sourceWorldId ||
			_partyService.FindPartyBySession(candidateSessionId) != 0)
		{
			continue;
		}

		selectedSessions.push_back(candidateSessionId);
		if (selectedSessions.size() >= MaxPartyMembers)
		{
			break;
		}
	}

	std::sort(selectedSessions.begin(), selectedSessions.end());

	return _partyService.CreatePartyFromTrustedMembers(
		actorSessionId,
		std::span<const SessionId>(
			selectedSessions.data(),
			selectedSessions.size()),
		PartyFormationSource::DemoAutoWorldTransition,
		nowSec);
}
