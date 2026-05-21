#include "pch.h"
#include "PartyCommandPump.h"

PartyCommandPump::PartyCommandPump(
	PartyCommandQueue& queue,
	PartyService& partyService,
	FrameworkRuntime& framework)
	: _queue(queue)
	, _partyService(partyService)
	, _framework(framework)
{
}

void PartyCommandPump::Pump(double nowSec)
{
	_partyService.ExpireJoinRequests(nowSec);

	_scratch.clear();
	_queue.DrainInto(_scratch);

	for (const PartyCommand& command : _scratch)
	{
		ApplyCommand(command, nowSec);
	}
}

void PartyCommandPump::ApplyCommand(
	const PartyCommand& command,
	double nowSec)
{
	switch (command.kind)
	{
	case PartyCommandKind::CreateParty:
		(void)_partyService.CreateParty(command.actorSessionId, nowSec);
		break;
	case PartyCommandKind::RequestJoin:
		(void)_partyService.RequestJoin(
			command.actorSessionId,
			command.partyId,
			nowSec);
		break;
	case PartyCommandKind::AcceptJoinRequest:
		(void)_partyService.AcceptJoinRequest(
			command.actorSessionId,
			command.requestId,
			nowSec);
		break;
	case PartyCommandKind::RejectJoinRequest:
		(void)_partyService.RejectJoinRequest(
			command.actorSessionId,
			command.requestId,
			nowSec);
		break;
	case PartyCommandKind::BeginWorldEntry:
		BeginWorldEntry(command, nowSec);
		break;
	case PartyCommandKind::TransferEnqueued:
		(void)_partyService.MarkWorldEntryEnqueued(
			command.partyId,
			command.transferId,
			nowSec);
		break;
	case PartyCommandKind::TransferCompleted:
		(void)_partyService.CompleteWorldEntry(
			command.partyId,
			command.transferId,
			command.targetWorldId,
			nowSec);
		break;
	case PartyCommandKind::TransferFailed:
		(void)_partyService.FailWorldEntry(
			command.partyId,
			command.transferId,
			nowSec);
		break;
	case PartyCommandKind::DbLoadCompleted:
	case PartyCommandKind::DbPersistCompleted:
		break;
	default:
		break;
	}
}

void PartyCommandPump::BeginWorldEntry(
	const PartyCommand& command,
	double nowSec)
{
	PartyWorldEntryResult entry =
		_partyService.BeginWorldEntry(
			command.actorSessionId,
			command.target,
			nowSec,
			command.allowFallback);
	if (!entry.Succeeded())
	{
		return;
	}

	if (!entry.request.target.targetWorldDefId.has_value())
	{
		(void)_partyService.FailWorldEntry(entry.partyId, 0, nowSec);
		return;
	}

	const TransferId transferId =
		_framework.RequestWorldTransfer(
			std::span<const SessionId>(
				entry.request.sessionIds.data(),
				entry.request.sessionIds.size()),
			entry.request.sourceWorldId,
			*entry.request.target.targetWorldDefId,
			entry.request.target.instanceKey,
			entry.request.partyId,
			entry.request.allowFallback,
			nowSec);
	if (transferId != 0)
	{
		(void)_partyService.MarkWorldEntryEnqueued(
			entry.partyId,
			transferId,
			nowSec);
	}
	else
	{
		(void)_partyService.FailWorldEntry(entry.partyId, 0, nowSec);
	}
}
