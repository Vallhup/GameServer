#pragma once

using namespace Protocol;

class ClientPartyState
{
public:
	void Clear();

	void ApplyUIBootstrap(const SC_PARTY_UI_BOOTSTRAP_PACKET& uiBootstrap);
	void ApplyListSnapshot(const SC_PARTY_LIST_SNAPSHOT_PACKET& listSnapshot);
	void ApplyCommandResult(const SC_PARTY_COMMAND_RESULT_PACKET& commandResult);
	void ApplySnapshot(const SC_PARTY_SNAPSHOT_PACKET& snapshot);
	void ApplyJoinRequestReceived(const SC_PARTY_JOIN_REQUEST_RECEIVED_PACKET& requestReceived);
	void ApplyJoinRequestClosed(const SC_PARTY_JOIN_REQUEST_CLOSED_PACKET& requestClosed);

	bool HasMyParty() const { return hasMyParty; }
	const PartySnapshot& GetMyParty() const { return myPartySnapshot; }
	const std::vector<PartyListEntry>& GetPartyList() const { return partyList; }
	uint64_t GetRevision() const { return revision; }
	uint32_t GetLastClientRequestId() const { return lastClientRequestId; }
	uint32_t GetLastError() const { return lastError; }
	bool LastCommandSucceeded() const { return lastCommandSuccess; }
	const std::vector<PartyJoinRequest>& GetPendingJoinRequests() const 
	{ 
		return pendingJoinRequests; 
	}

private:
	static bool IsActiveParty(const PartySnapshot& party);

	static void ReplacePartyList(
		std::vector<PartyListEntry>& out,
		const google::protobuf::RepeatedPtrField<PartyListEntry>& parties);

	static void RebuildPendingJoinRequests(
		std::vector<PartyJoinRequest>& out,
		const PartySnapshot& party);

private:
	bool hasMyParty{ false };
	PartySnapshot myPartySnapshot;
	std::vector<PartyListEntry> partyList;
	std::vector<PartyJoinRequest> pendingJoinRequests;

	uint64_t revision{ 0 };
	uint32_t lastClientRequestId{ 0 };
	uint32_t lastError{ 0 };
	bool lastCommandSuccess{ false };
};
