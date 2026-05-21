#pragma once

#include <vector>

#include "FrameworkRuntime.h"
#include "NetworkRuntime.h"
#include "PartyCommandQueue.h"
#include "PartyService.h"

class PartyCommandPump final
{
public:
	PartyCommandPump(
		PartyCommandQueue& queue,
		PartyService& partyService,
		FrameworkRuntime& framework,
		NetworkRuntime& network);

	void Pump(double nowSec);

private:
	void ApplyCommand(const PartyCommand& command, double nowSec);
	void SendUiBootstrap(const PartyCommand& command);
	void SendListSnapshot(const PartyCommand& command);
	void CreateParty(const PartyCommand& command, double nowSec);
	void RequestJoin(const PartyCommand& command, double nowSec);
	void AcceptJoinRequest(const PartyCommand& command, double nowSec);
	void RejectJoinRequest(const PartyCommand& command, double nowSec);
	void MarkMemberOffline(const PartyCommand& command, double nowSec);
	void BeginWorldEntry(const PartyCommand& command, double nowSec);
	void StagePartySnapshotToMembers(const PartySnapshot& snapshot);

private:
	PartyCommandQueue& _queue;
	PartyService& _partyService;
	FrameworkRuntime& _framework;
	NetworkRuntime& _network;
	std::vector<PartyCommand> _scratch;
	std::vector<PartyListEntry> _listScratch;
};
