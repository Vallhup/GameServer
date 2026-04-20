#include "pch.h"
#include "ClientWorldTransitionController.h"

uint32_t ClientWorldTransitionController::CreateRequestId()
{
	if (nextRequestId == 0) nextRequestId = 1;
	return nextRequestId++;
}

bool ClientWorldTransitionController::BeginRequest(uint32_t requestId)
{
	if (phase != ClientWorldTransitionPhase::Idle &&
		phase != ClientWorldTransitionPhase::Rejected)
	{
		return false;
	}

	context = {};
	context.requestId = requestId;
	
	phase = ClientWorldTransitionPhase::RequestSent;
	return true;
}

bool ClientWorldTransitionController::OnBegin(const Protocol::SC_WORLD_TRANSITION_BEGIN_PACKET& packet)
{
	// TEMP: requestId가 0이 될 수 있을지 없을지 확정 X
	if (phase == ClientWorldTransitionPhase::RequestSent &&
		packet.requestid() != 0 &&
		context.requestId != packet.requestid())
	{
		return false;
	}

	context.requestId = packet.requestid();
	context.transferId = packet.transferid();
	context.sourceWorldDefId = packet.sourceworlddefid();
	context.sourceWorldId = packet.sourceworldid();
	context.targetWorldDefId = packet.targetworlddefid();
	context.targetWorldId = packet.targetworldid();
	context.mapResourceId = packet.mapresourceid();
	context.playerNetId = packet.playernetid();
	context.clearExistingObjects = packet.clearexistingobjects();
	context.waitClientReady = packet.waitclientready();
	context.usedFallback = packet.usedfallback();
	context.reason = packet.reason();

	phase = ClientWorldTransitionPhase::BeginReceived;
	return true;
}

bool ClientWorldTransitionController::OnRejected(const Protocol::SC_WORLD_TRANSITION_REJECTED_PACKET& packet)
{
	if (phase == ClientWorldTransitionPhase::RequestSent &&
		packet.requestid() != 0 &&
		context.requestId != packet.requestid())
	{
		return false;
	}

	context.requestId = packet.requestid();
	context.reason = packet.reason();

	phase = ClientWorldTransitionPhase::Rejected;
	return true;
}

void ClientWorldTransitionController::MarkLoadingStarted()
{
	if (phase == ClientWorldTransitionPhase::BeginReceived)
	{
		phase = ClientWorldTransitionPhase::Loading;
	}
}

void ClientWorldTransitionController::MarkReadySent()
{
	if (phase == ClientWorldTransitionPhase::BeginReceived ||
		phase == ClientWorldTransitionPhase::Loading)
	{
		phase = ClientWorldTransitionPhase::ReadySent;
	}
}

void ClientWorldTransitionController::Complete()
{
	if (phase == ClientWorldTransitionPhase::ReadySent)
	{
		Reset();
	}
}

void ClientWorldTransitionController::Reset()
{
	context = {};
	phase = ClientWorldTransitionPhase::Idle;
}

bool ClientWorldTransitionController::HasBeginContext() const
{
	return 
		context.transferId != 0 &&
		(phase == ClientWorldTransitionPhase::BeginReceived ||
		 phase == ClientWorldTransitionPhase::Loading ||
		 phase == ClientWorldTransitionPhase::ReadySent);
}

bool ClientWorldTransitionController::HasPendingReady() const
{
	return
		context.transferId != 0 &&
		context.waitClientReady &&
		(phase == ClientWorldTransitionPhase::BeginReceived ||
		 phase == ClientWorldTransitionPhase::Loading);
}
