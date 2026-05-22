#include "pch.h"
#include "ClientPartyState.h"

#include <algorithm>

void ClientPartyState::Clear()
{
	hasMyParty = false;
	myPartySnapshot.Clear();
	partyList.clear();
	pendingJoinRequests.clear();
	lastClientRequestId = 0;
	lastError = 0;
	lastCommandSuccess = false;
	++revision;
}

void ClientPartyState::ApplyUIBootstrap(const SC_PARTY_UI_BOOTSTRAP_PACKET& uiBootstrap)
{
	hasMyParty = uiBootstrap.hasmyparty() && IsActiveParty(uiBootstrap.myparty());
	if (hasMyParty)
	{
		myPartySnapshot = uiBootstrap.myparty();
		RebuildPendingJoinRequests(pendingJoinRequests, myPartySnapshot);
	}
	else
	{
		myPartySnapshot.Clear();
		pendingJoinRequests.clear();
	}

	ReplacePartyList(partyList, uiBootstrap.parties());
	lastClientRequestId = uiBootstrap.clientrequestid();
	++revision;
}

void ClientPartyState::ApplyListSnapshot(const SC_PARTY_LIST_SNAPSHOT_PACKET& listSnapshot)
{
	ReplacePartyList(partyList, listSnapshot.parties());
	lastClientRequestId = listSnapshot.clientrequestid();
	++revision;
}

void ClientPartyState::ApplyCommandResult(const SC_PARTY_COMMAND_RESULT_PACKET& commandResult)
{
	lastClientRequestId = commandResult.clientrequestid();
	lastCommandSuccess = commandResult.success();
	lastError = commandResult.error();
	++revision;
}

void ClientPartyState::ApplySnapshot(const SC_PARTY_SNAPSHOT_PACKET& snapshot)
{
	hasMyParty = IsActiveParty(snapshot.party());
	if (hasMyParty)
	{
		myPartySnapshot = snapshot.party();
		RebuildPendingJoinRequests(pendingJoinRequests, myPartySnapshot);
	}
	else
	{
		myPartySnapshot.Clear();
		pendingJoinRequests.clear();
	}

	++revision;
}

void ClientPartyState::ApplyJoinRequestReceived(const SC_PARTY_JOIN_REQUEST_RECEIVED_PACKET& requestReceived)
{
	const PartyJoinRequest& request = requestReceived.request();
	const uint64_t requestId = request.joinrequestid();

	const auto pendingIt = std::find_if(
		pendingJoinRequests.begin(),
		pendingJoinRequests.end(),
		[requestId](const PartyJoinRequest& existing)
		{
			return existing.joinrequestid() == requestId;
		});

	if (request.state() == PARTY_JOIN_REQUEST_PENDING)
	{
		if (pendingIt != pendingJoinRequests.end())
		{
			*pendingIt = request;
		}
		else
		{
			pendingJoinRequests.push_back(request);
		}
	}
	else if (pendingIt != pendingJoinRequests.end())
	{
		pendingJoinRequests.erase(pendingIt);
	}

	if (hasMyParty &&
		myPartySnapshot.partyid() == requestReceived.partyid())
	{
		for (int i = 0; i < myPartySnapshot.joinrequests_size(); ++i)
		{
			if (myPartySnapshot.joinrequests(i).joinrequestid() == requestId)
			{
				*myPartySnapshot.mutable_joinrequests(i) = request;
				++revision;
				return;
			}
		}

		*myPartySnapshot.add_joinrequests() = request;
	}

	++revision;
}

void ClientPartyState::ApplyJoinRequestClosed(const SC_PARTY_JOIN_REQUEST_CLOSED_PACKET& requestClosed)
{
	const uint64_t requestId = requestClosed.joinrequestid();

	std::erase_if(pendingJoinRequests,
		[requestId](const PartyJoinRequest& request)
		{
			return request.joinrequestid() == requestId;
		});

	if (hasMyParty &&
		myPartySnapshot.partyid() == requestClosed.partyid())
	{
		for (int i = 0; i < myPartySnapshot.joinrequests_size(); ++i)
		{
			if (myPartySnapshot.joinrequests(i).joinrequestid() == requestId)
			{
				PartyJoinRequest* request = myPartySnapshot.mutable_joinrequests(i);
				request->set_state(requestClosed.state());
				request->set_closereason(requestClosed.reason());
				break;
			}
		}
	}

	++revision;
}

bool ClientPartyState::IsActiveParty(const PartySnapshot& party)
{
	return party.partyid() != 0 &&
		party.lifecycle() != PARTY_LIFECYCLE_DISBANDED;
}

void ClientPartyState::ReplacePartyList(
	std::vector<PartyListEntry>& out,
	const google::protobuf::RepeatedPtrField<PartyListEntry>& parties)
{
	out.clear();
	out.reserve(parties.size());
	for (const PartyListEntry& party : parties)
	{
		out.push_back(party);
	}
}

void ClientPartyState::RebuildPendingJoinRequests(
	std::vector<PartyJoinRequest>& out,
	const PartySnapshot& party)
{
	out.clear();
	out.reserve(party.joinrequests_size());
	for (const PartyJoinRequest& request : party.joinrequests())
	{
		if (request.state() == PARTY_JOIN_REQUEST_PENDING)
		{
			out.push_back(request);
		}
	}
}
