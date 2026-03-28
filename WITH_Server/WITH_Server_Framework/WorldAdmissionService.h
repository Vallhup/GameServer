#pragma once

#include <unordered_map>
#include <cstdint>

#include "AdmissionTypes.h"
#include "WorldManager.h"

class WorldAdmissionService final {
public:
	explicit WorldAdmissionService(
		WorldManager& worldMng,
		PresenceManager& presenceMng
	);

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

	bool ValidateReEntryRequest(const AdmissionRequest& request) const;

	AdmissionResult MakeRejectedResult(AdmissionDecision decision) const;
	uint64_t AllocateTicket();

private:
	WorldManager& _worldMng;
	PresenceManager& _presenceMng;

	uint64_t _nextTicket{ 1 };
	std::unordered_map<uint64_t, AdmissionReservation> _reservations;

	// 정책값 : 나중에 config로 분리 가능
	uint32_t _maxInflightTransferInPerWorld{ 1024 };
	double _defaultReservationTtlSec{ 5.0 };
};

// reEntryEligible를 언제 true/false로 세팅할지에 대한 운영 정책
// 
// Disconnected 상태를 다시 Active로 되돌리는 최종 확정은
// attach / login flow와 붙여야 완성됨
// 
// 파티 단위 reEntry는 다루지 않음
// 
// AdmissionReason::Login, Respawn에 대한 presence 검증 없음

