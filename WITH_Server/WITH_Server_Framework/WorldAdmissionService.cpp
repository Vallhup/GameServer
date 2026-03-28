#include "pch.h"
#include "WorldAdmissionService.h"

WorldAdmissionService::WorldAdmissionService(WorldManager& worldManager)
	: _worldMng(worldManager)
{
}

AdmissionResult WorldAdmissionService::TryReserve(const AdmissionRequest& request, double nowSec)
{
	if (!request.IsValid())
		return MakeRejectedResult(AdmissionDecision::RejectedStage);

	WorldInstanceRecord* record = _worldMng.FindRecord(request.targetWorldId);
	if (record == nullptr)
		return MakeRejectedResult(AdmissionDecision::RejectedStage);

	const AdmissionDecision decision = EvaluateRequest(request, *record);
	if (decision != AdmissionDecision::Accepted)
		return MakeRejectedResult(decision);

	AdmissionReservation reservation;
	reservation.stage = AdmissionReservationStage::Reserved;
	reservation.reason = request.reason;
	reservation.ticket = AllocateTicket();
	reservation.targetWorldId = request.targetWorldId;
	reservation.partyId = request.partyId;
	reservation.reservedSlots = request.RequestedPlayerCount();
	reservation.createdAtSec = nowSec;
	reservation.expiresAtSec = nowSec + _defaultReservationTtlSec;

	if (!_worldMng.AddReservedSlots(record->id, reservation.reservedSlots))
		return MakeRejectedResult(AdmissionDecision::RejectedCapacity);

	_reservations.try_emplace(reservation.ticket, reservation);

	AdmissionResult result;
	result.decision = AdmissionDecision::Accepted;
	result.reservation = reservation;
	return result;
}

bool WorldAdmissionService::ConsumeReservation(uint64_t ticket, double nowSec)
{
	AdmissionReservation* reservation = FindReservation(ticket);
	if (reservation == nullptr)
		return false;

	if (!reservation->IsActiveReservation())
		return false;

	if (reservation->IsExpired(nowSec))
	{
		if (_worldMng.RemoveReservedSlots(
			reservation->targetWorldId,
			reservation->reservedSlots))
		{
			reservation->stage = AdmissionReservationStage::Expired;
		}
		return false;
	}

	if (!_worldMng.RemoveReservedSlots(
		reservation->targetWorldId,
		reservation->reservedSlots))
	{
		return false;
	}

	reservation->stage = AdmissionReservationStage::Consumed;
	return true;
}

bool WorldAdmissionService::CancelReservation(uint64_t ticket)
{
	AdmissionReservation* reservation = FindReservation(ticket);
	if (reservation == nullptr)
		return false;

	if (reservation->stage != AdmissionReservationStage::Reserved)
		return false;

	if (!_worldMng.RemoveReservedSlots(
		reservation->targetWorldId,
		reservation->reservedSlots))
	{
		return false;
	}

	reservation->stage = AdmissionReservationStage::Cancelled;
	return true;
}

void WorldAdmissionService::ExpireReservations(double nowSec)
{
	for (auto& [_, reservation] : _reservations)
	{
		if (reservation.stage != AdmissionReservationStage::Reserved)
			continue;

		if (!reservation.IsExpired(nowSec))
			continue;

		if (_worldMng.RemoveReservedSlots(
			reservation.targetWorldId,
			reservation.reservedSlots))
		{
			reservation.stage = AdmissionReservationStage::Expired;
		}
	}
}

AdmissionReservation* WorldAdmissionService::FindReservation(uint64_t ticket)
{
	auto it = _reservations.find(ticket);
	if (it == _reservations.end())
		return nullptr;

	return &it->second;
}

const AdmissionReservation* WorldAdmissionService::FindReservation(uint64_t ticket) const
{
	auto it = _reservations.find(ticket);
	if (it == _reservations.end())
		return nullptr;

	return &it->second;
}

AdmissionDecision WorldAdmissionService::EvaluateRequest(
	const AdmissionRequest& request,
	const WorldInstanceRecord& record) const
{
	if (!record.IsRunnable())
		return AdmissionDecision::RejectedStage;

	if (record.IsClosingLike())
		return AdmissionDecision::RejectedClosing;

	// 현재 request에는 party / matchmaking context가 없으므로
	// FreeJoin 외 정책은 보수적으로 거절한다.
	switch (record.joinPolicy) {
	case JoinPolicy::FreeJoin:
	{
		break;
	}
	case JoinPolicy::PartyOnly:
	{
		if (!request.HasPartyContext())
			return AdmissionDecision::RejectedJoinPolicy;
		break;
	}
	default:
	{
		return AdmissionDecision::RejectedJoinPolicy;
	}
	}

	if (request.reason == AdmissionReason::ReEntry && !record.allowReEntry)
		return AdmissionDecision::RejectedReEntry;

	if (!record.CanReserve(request.RequestedPlayerCount()))
		return AdmissionDecision::RejectedCapacity;

	if (record.inFlightTransfersIn + request.RequestedPlayerCount() >
		_maxInflightTransferInPerWorld)
	{
		return AdmissionDecision::RejectedTransferOverflow;
	}

	return AdmissionDecision::Accepted;
}

AdmissionResult WorldAdmissionService::MakeRejectedResult(AdmissionDecision decision) const
{
	AdmissionResult result;
	result.decision = decision;
	return result;
}

uint64_t WorldAdmissionService::AllocateTicket()
{
	return _nextTicket++;
}