#pragma once

#include <unordered_map>
#include <cstdint>

#include "AdmissionTypes.h"
#include "WorldManager.h"

class WorldAdmissionService final {
public:
	explicit WorldAdmissionService(WorldManager& worldMng);

	AdmissionResult TryReserve(const AdmissionRequest& request, const double nowSec);

	bool ConsumeReservation(uint64_t ticket, const double nowSec);
	bool CancelReservation(uint64_t ticket);
	void ExpireReservations(const double nowSec);

	AdmissionReservation* FindReservation(uint64_t ticket);
	const AdmissionReservation* FindReservation(uint64_t ticket) const;

private:
	AdmissionDecision EvaluateRequest(
		const AdmissionRequest& request,
		const WorldInstanceRecord& record
	) const;

	AdmissionResult MakeRejectedResult(AdmissionDecision decision) const;
	uint64_t AllocateTicket();

private:
	WorldManager& _worldMng;

	uint64_t _nextTicket{ 1 };
	std::unordered_map<uint64_t, AdmissionReservation> _reservations;

	// 정책값 : 나중에 config로 분리 가능
	uint32_t _maxInflightTransferInPerWorld{ 1024 };
	double _defaultReservationTtlSec{ 5.0 };
};

