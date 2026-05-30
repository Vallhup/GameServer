#include "pch.h"
#include "PartyCommandPump.h"

#include <algorithm>

#include "FrameworkLog.h"
#include "PartyPersistGateway.h"
#include "ServerPacketStager.h"

namespace
{
	constexpr const char* kLogCategory = "Party";
}

PartyCommandPump::PartyCommandPump(
	PartyCommandQueue& queue,
	PartyService& partyService,
	FrameworkRuntime& framework,
	NetworkRuntime& network)
	: _queue(queue)
	, _partyService(partyService)
	, _framework(framework)
	, _network(network)
{
}

void PartyCommandPump::SetPersistGateway(
	PartyPersistGateway* persistGateway) noexcept
{
	_persistGateway = persistGateway;
}

void PartyCommandPump::Pump(double nowSec)
{
	if (_persistGateway != nullptr)
	{
		_persistGateway->Process(nowSec);
	}

	_partyService.ExpireJoinRequests(nowSec);

	_scratch.clear();
	_queue.DrainInto(_scratch);
	if (!_scratch.empty())
	{
		FWLOG_INFO(
			kLogCategory,
			"PartyCommandPump drained commands (count=%zu, nowSec=%.3f)",
			_scratch.size(),
			nowSec);
	}

	for (const PartyCommand& command : _scratch)
	{
		ApplyCommand(command, nowSec);
	}
}

void PartyCommandPump::ApplyCommand(
	const PartyCommand& command,
	double nowSec)
{
	FWLOG_INFO(
		kLogCategory,
		"ApplyCommand dispatch (kind=%d, sid=%u, partyId=%llu, requestId=%llu, clientRequestId=%u, nowSec=%.3f)",
		static_cast<int>(command.kind),
		command.actorSessionId,
		static_cast<unsigned long long>(command.partyId),
		static_cast<unsigned long long>(command.requestId),
		command.clientRequestId,
		nowSec);

	switch (command.kind)
	{
	case PartyCommandKind::UiOpened:
		SendUiBootstrap(command);
		break;
	case PartyCommandKind::UiClosed:
		RemovePartyListSubscriber(command.actorSessionId);
		break;
	case PartyCommandKind::ListRefresh:
		SendListSnapshot(command);
		break;
	case PartyCommandKind::CreateParty:
		CreateParty(command, nowSec);
		break;
	case PartyCommandKind::RequestJoin:
		RequestJoin(command, nowSec);
		break;
	case PartyCommandKind::AcceptJoinRequest:
		AcceptJoinRequest(command, nowSec);
		break;
	case PartyCommandKind::RejectJoinRequest:
		RejectJoinRequest(command, nowSec);
		break;
	case PartyCommandKind::MarkMemberOffline:
		MarkMemberOffline(command, nowSec);
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
		if (_partyService.CompleteWorldEntry(
			command.partyId,
			command.transferId,
			command.targetWorldId,
			nowSec).Succeeded())
		{
			PersistPartyState(command.partyId, nowSec);
		}
		break;
	case PartyCommandKind::TransferFailed:
		if (_partyService.FailWorldEntry(
			command.partyId,
			command.transferId,
			nowSec).Succeeded())
		{
			PersistPartyState(command.partyId, nowSec);
			BroadcastPartyListSnapshot();
		}
		break;
	case PartyCommandKind::RebindRestoredMember:
		RebindRestoredMember(command, nowSec);
		break;
	case PartyCommandKind::DbLoadCompleted:
	case PartyCommandKind::DbPersistCompleted:
		// 복구/저장 완료는 PartyPersistGateway가 main tick에서 직접 흡수하므로
		// 여기서는 별도 처리가 필요 없다.
		break;
	default:
		break;
	}
}

void PartyCommandPump::SendUiBootstrap(const PartyCommand& command)
{
	FWLOG_INFO(
		kLogCategory,
		"SendUiBootstrap begin (sid=%u, clientRequestId=%u)",
		command.actorSessionId,
		command.clientRequestId);

	RegisterPartyListSubscriber(command.actorSessionId);

	PartySnapshot snapshot =
		_partyService.BuildPartySnapshotForSession(command.actorSessionId);
	_listScratch.clear();
	_partyService.CollectPublicPartyList(_listScratch);

	const PartySnapshot* const myParty =
		snapshot.partyId != 0 ? &snapshot : nullptr;
	const bool staged = ServerPacketStager::StagePartyUiBootstrapPacket(
		_network,
		command.actorSessionId,
		command.clientRequestId,
		myParty,
		std::span<const PartyListEntry>(
			_listScratch.data(),
			_listScratch.size()));
	FWLOG_INFO(
		kLogCategory,
		"SendUiBootstrap staged (sid=%u, clientRequestId=%u, hasMyParty=%u, partyCount=%zu, staged=%u)",
		command.actorSessionId,
		command.clientRequestId,
		myParty != nullptr ? 1u : 0u,
		_listScratch.size(),
		staged ? 1u : 0u);
}

void PartyCommandPump::SendListSnapshot(const PartyCommand& command)
{
	RegisterPartyListSubscriber(command.actorSessionId);

	_listScratch.clear();
	_partyService.CollectPublicPartyList(_listScratch);
	(void)ServerPacketStager::StagePartyListSnapshotPacket(
		_network,
		command.actorSessionId,
		command.clientRequestId,
		std::span<const PartyListEntry>(
			_listScratch.data(),
			_listScratch.size()));
}

void PartyCommandPump::CreateParty(
	const PartyCommand& command,
	double nowSec)
{
	const PartyResult result =
		_partyService.CreateParty(command.actorSessionId, nowSec);
	(void)ServerPacketStager::StagePartyCommandResultPacket(
		_network,
		command.actorSessionId,
		command.clientRequestId,
		result);

	if (result.Succeeded())
	{
		(void)ServerPacketStager::StagePartySnapshotPacketToSession(
			_network,
			command.actorSessionId,
			_partyService.BuildPartySnapshot(result.partyId));
		PersistPartyState(result.partyId, nowSec);
		BroadcastPartyListSnapshot();
	}
}

void PartyCommandPump::RequestJoin(
	const PartyCommand& command,
	double nowSec)
{
	FWLOG_INFO(
		kLogCategory,
		"JoinRequest received (sid=%u, partyId=%llu, clientRequestId=%u)",
		command.actorSessionId,
		static_cast<unsigned long long>(command.partyId),
		command.clientRequestId);

	const PartyResult result =
		_partyService.RequestJoin(
			command.actorSessionId,
			command.partyId,
			nowSec);
	const bool resultStaged =
		ServerPacketStager::StagePartyCommandResultPacket(
		_network,
		command.actorSessionId,
		command.clientRequestId,
		result);
	if (!resultStaged)
	{
		FWLOG_WARN(
			kLogCategory,
			"JoinRequest result stage failed (sid=%u, partyId=%llu, requestId=%llu, error=%u)",
			command.actorSessionId,
			static_cast<unsigned long long>(command.partyId),
			static_cast<unsigned long long>(result.requestId),
			static_cast<uint32_t>(result.error));
	}

	if (!result.Succeeded())
	{
		FWLOG_WARN(
			kLogCategory,
			"JoinRequest rejected (sid=%u, partyId=%llu, error=%u, clientRequestId=%u)",
			command.actorSessionId,
			static_cast<unsigned long long>(command.partyId),
			static_cast<uint32_t>(result.error),
			command.clientRequestId);
		return;
	}

	const PartySnapshot snapshot =
		_partyService.BuildPartySnapshot(result.partyId);
	const auto requestIt = std::find_if(
		snapshot.joinRequests.begin(),
		snapshot.joinRequests.end(),
		[&result](const PartyJoinRequestSnapshot& request)
		{
			return request.requestId == result.requestId;
		});
	if (requestIt == snapshot.joinRequests.end())
	{
		FWLOG_WARN(
			kLogCategory,
			"JoinRequest snapshot missing (sid=%u, partyId=%llu, requestId=%llu, leaderSid=%u)",
			command.actorSessionId,
			static_cast<unsigned long long>(result.partyId),
			static_cast<unsigned long long>(result.requestId),
			snapshot.leaderSessionId);
		return;
	}

	const bool notifyStaged =
		ServerPacketStager::StagePartyJoinRequestReceivedPacket(
		_network,
		snapshot.leaderSessionId,
		result.partyId,
		*requestIt);
	FWLOG_INFO(
		kLogCategory,
		"JoinRequest accepted (sid=%u, partyId=%llu, requestId=%llu, leaderSid=%u, notifyStaged=%u)",
		command.actorSessionId,
		static_cast<unsigned long long>(result.partyId),
		static_cast<unsigned long long>(result.requestId),
		snapshot.leaderSessionId,
		notifyStaged ? 1u : 0u);
	if (!notifyStaged)
	{
		FWLOG_WARN(
			kLogCategory,
			"JoinRequest leader notify stage failed (sid=%u, partyId=%llu, requestId=%llu, leaderSid=%u)",
			command.actorSessionId,
			static_cast<unsigned long long>(result.partyId),
			static_cast<unsigned long long>(result.requestId),
			snapshot.leaderSessionId);
	}
}

void PartyCommandPump::AcceptJoinRequest(
	const PartyCommand& command,
	double nowSec)
{
	const PartyJoinRequest* const requestBefore =
		_partyService.FindJoinRequest(command.requestId);
	const SessionId requesterSessionId =
		requestBefore != nullptr ? requestBefore->requesterSessionId : 0;

	const PartyResult result =
		_partyService.AcceptJoinRequest(
			command.actorSessionId,
			command.requestId,
			nowSec);
	(void)ServerPacketStager::StagePartyCommandResultPacket(
		_network,
		command.actorSessionId,
		command.clientRequestId,
		result);

	if (!result.Succeeded())
	{
		return;
	}

	const PartySnapshot snapshot =
		_partyService.BuildPartySnapshot(result.partyId);
	const auto requestIt = std::find_if(
		snapshot.joinRequests.begin(),
		snapshot.joinRequests.end(),
		[&result](const PartyJoinRequestSnapshot& request)
		{
			return request.requestId == result.requestId;
		});
	if (requesterSessionId != 0 && requestIt != snapshot.joinRequests.end())
	{
		(void)ServerPacketStager::StagePartyJoinRequestClosedPacket(
			_network,
			requesterSessionId,
			result.partyId,
			*requestIt);
	}

	StagePartySnapshotToMembers(snapshot);
	PersistPartyState(result.partyId, nowSec);
	BroadcastPartyListSnapshot();
}

void PartyCommandPump::RejectJoinRequest(
	const PartyCommand& command,
	double nowSec)
{
	const PartyJoinRequest* const requestBefore =
		_partyService.FindJoinRequest(command.requestId);
	const SessionId requesterSessionId =
		requestBefore != nullptr ? requestBefore->requesterSessionId : 0;

	const PartyResult result =
		_partyService.RejectJoinRequest(
			command.actorSessionId,
			command.requestId,
			nowSec);
	(void)ServerPacketStager::StagePartyCommandResultPacket(
		_network,
		command.actorSessionId,
		command.clientRequestId,
		result);

	if (!result.Succeeded())
	{
		return;
	}

	const PartySnapshot snapshot =
		_partyService.BuildPartySnapshot(result.partyId);
	const auto requestIt = std::find_if(
		snapshot.joinRequests.begin(),
		snapshot.joinRequests.end(),
		[&result](const PartyJoinRequestSnapshot& request)
		{
			return request.requestId == result.requestId;
		});
	if (requesterSessionId != 0 && requestIt != snapshot.joinRequests.end())
	{
		(void)ServerPacketStager::StagePartyJoinRequestClosedPacket(
			_network,
			requesterSessionId,
			result.partyId,
			*requestIt);
	}

	(void)ServerPacketStager::StagePartySnapshotPacketToSession(
		_network,
		command.actorSessionId,
		snapshot);
}

void PartyCommandPump::MarkMemberOffline(
	const PartyCommand& command,
	double nowSec)
{
	RemovePartyListSubscriber(command.actorSessionId);

	const PartyId partyId =
		_partyService.FindPartyBySession(command.actorSessionId);
	const PartyResult result =
		_partyService.MarkMemberPresence(
			command.actorSessionId,
			PartyMemberPresence::Offline,
			nowSec);
	if (!result.Succeeded() || partyId == 0)
	{
		return;
	}

	const PartySnapshot snapshot =
		_partyService.BuildPartySnapshot(partyId);
	if (snapshot.partyId != 0)
	{
		StagePartySnapshotToMembers(snapshot);
	}

	PersistPartyState(partyId, nowSec);
	BroadcastPartyListSnapshot();
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
		const PartyResult enqueueResult =
			_partyService.MarkWorldEntryEnqueued(
			entry.partyId,
			transferId,
			nowSec);
		if (enqueueResult.Succeeded())
		{
			PersistPartyState(entry.partyId, nowSec);
			BroadcastPartyListSnapshot();
		}
	}
	else
	{
		(void)_partyService.FailWorldEntry(entry.partyId, 0, nowSec);
		PersistPartyState(entry.partyId, nowSec);
	}
}

void PartyCommandPump::RebindRestoredMember(
	const PartyCommand& command,
	double nowSec)
{
	const PartyResult result =
		_partyService.RebindMemberByAccount(
			command.correlationId,
			command.actorSessionId,
			nowSec);
	if (!result.Succeeded())
	{
		return;
	}

	const PartySnapshot snapshot =
		_partyService.BuildPartySnapshot(result.partyId);
	if (snapshot.partyId != 0)
	{
		StagePartySnapshotToMembers(snapshot);
	}

	PersistPartyState(result.partyId, nowSec);
	BroadcastPartyListSnapshot();
}

void PartyCommandPump::PersistPartyState(PartyId partyId, double nowSec)
{
	if (_persistGateway == nullptr)
	{
		return;
	}

	_persistGateway->PersistCurrentState(partyId, nowSec);
}

void PartyCommandPump::StagePartySnapshotToMembers(
	const PartySnapshot& snapshot)
{
	std::vector<SessionId> sessionIds;
	sessionIds.reserve(snapshot.members.size());
	for (const PartyMemberSnapshot& member : snapshot.members)
	{
		if (member.presence == PartyMemberPresence::Online)
		{
			sessionIds.push_back(member.sessionId);
		}
	}

	(void)ServerPacketStager::StagePartySnapshotPacketToSessions(
		_network,
		std::span<const SessionId>(sessionIds.data(), sessionIds.size()),
		snapshot);
}

void PartyCommandPump::RegisterPartyListSubscriber(SessionId sessionId)
{
	if (sessionId == 0)
	{
		return;
	}

	if (std::find(
		_partyListSubscribers.begin(),
		_partyListSubscribers.end(),
		sessionId) == _partyListSubscribers.end())
	{
		_partyListSubscribers.push_back(sessionId);
	}
}

void PartyCommandPump::RemovePartyListSubscriber(SessionId sessionId)
{
	_partyListSubscribers.erase(
		std::remove(
			_partyListSubscribers.begin(),
			_partyListSubscribers.end(),
			sessionId),
		_partyListSubscribers.end());
}

void PartyCommandPump::BroadcastPartyListSnapshot()
{
	if (_partyListSubscribers.empty())
	{
		return;
	}

	_listScratch.clear();
	_partyService.CollectPublicPartyList(_listScratch);

	for (auto it = _partyListSubscribers.begin();
		it != _partyListSubscribers.end();)
	{
		const bool staged =
			ServerPacketStager::StagePartyListSnapshotPacket(
				_network,
				*it,
				0,
				std::span<const PartyListEntry>(
					_listScratch.data(),
					_listScratch.size()));
		if (!staged)
		{
			it = _partyListSubscribers.erase(it);
		}
		else
		{
			++it;
		}
	}
}
