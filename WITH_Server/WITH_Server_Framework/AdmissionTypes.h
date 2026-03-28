#pragma once

#include <cstdint>
#include <vector>

#include "WorldId.h"
#include "WorldIds.h"
#include "WorldLifecycleEnums.h"

enum class AdmissionReason : uint8_t
{
	Login,
	Transfer,
	Respawn,
	ReEntry
};

enum class AdmissionReservationStage : uint8_t
{
	None,
	Reserved,
	Consumed,
	Expired,
	Cancelled
};

struct AdmissionRequest
{
	WorldId targetWorldId{ WorldId::Invalid() };
	std::vector<uint32_t> connectionIds;
	AdmissionReason reason{ AdmissionReason::Transfer };

	PartyId partyId{ 0 };

	inline bool IsValid() const
	{
		return targetWorldId.IsValid() && !connectionIds.empty();
	}

	inline uint32_t RequestedPlayerCount() const
	{
		return static_cast<uint32_t>(connectionIds.size());
	}

	inline bool HasPartyContext() const
	{
		return partyId != 0;
	}
};

struct AdmissionReservation
{
	AdmissionReservationStage stage{ AdmissionReservationStage::None };
	AdmissionReason reason{ AdmissionReason::Transfer };
	ReservationTicket ticket{ 0 };

	WorldId targetWorldId{ WorldId::Invalid() };

	// PartyOnly world에서 발급된 reservation이면 0이 아니어야 함
	PartyId partyId{ 0 };

	// 기본 정책
	// reservedSlots == request.connectionIds.size()
	uint32_t reservedSlots{ 0 };

	double createdAtSec{ 0.0 };
	double expiresAtSec{ 0.0 };

	inline bool IsActiveReservation() const
	{
		return
			stage == AdmissionReservationStage::Reserved &&
			ticket != 0 &&
			targetWorldId.IsValid() &&
			reservedSlots > 0 &&
			expiresAtSec > 0.0;
	}

	inline bool IsExpired(const double nowSec) const
	{
		return 
			stage == AdmissionReservationStage::Reserved &&
			expiresAtSec > 0.0 && 
			nowSec >= expiresAtSec;
	}
};

struct AdmissionResult
{
	// decision == Accepted 인 경우에만 reservation을 신뢰한다.
	// rejected면 reservation은 기본값 상태여야 한다.
	// IsAccepted() 없이 reservation을 바로 쓰지 않는다.
	AdmissionDecision decision{ AdmissionDecision::RejectedStage };
	AdmissionReservation reservation;

	inline bool IsAccepted() const
	{
		return decision == AdmissionDecision::Accepted;
	}
};