#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

#include "PartyTypes.h"

enum class PartyCommandKind : uint8_t
{
	UiOpened,
	UiClosed,
	ListRefresh,
	CreateParty,
	RequestJoin,
	AcceptJoinRequest,
	RejectJoinRequest,
	MarkMemberOffline,
	BeginWorldEntry,
	TransferEnqueued,
	TransferCompleted,
	TransferFailed,
	RebindRestoredMember,
	DbLoadCompleted,
	DbPersistCompleted
};

struct PartyCommand
{
	PartyCommandKind kind{ PartyCommandKind::CreateParty };
	SessionId actorSessionId{ 0 };
	PartyId partyId{ 0 };
	PartyRequestId requestId{ 0 };
	TransferId transferId{ 0 };
	WorldId targetWorldId{ WorldId::Invalid() };
	WorldTargetSpec target;
	bool allowFallback{ false };
	uint32_t clientRequestId{ 0 };
	double submittedAtSec{ 0.0 };
	uint64_t correlationId{ 0 };
};

class PartyCommandQueue final
{
public:
	void Submit(PartyCommand command);
	void DrainInto(std::vector<PartyCommand>& out);
	void Clear();
	bool Empty() const;

private:
	mutable std::mutex _mutex;
	std::deque<PartyCommand> _commands;
};
