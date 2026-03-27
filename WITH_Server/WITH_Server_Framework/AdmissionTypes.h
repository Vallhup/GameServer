#pragma once

#include <cstdint>
#include <vector>

#include "WorldId.h"
#include "WorldLifecycleEnums.h"

enum class AdmissionReason : uint8_t
{
	Login,
	Transfer,
	Respawn,
	ReEntry
};

struct AdmissionRequest
{
	WorldId targetWorldId{ 0 };
	std::vector<uint32_t> connectionIds;
	AdmissionReason reason{ AdmissionReason::Transfer };
};

struct AdmissionReservation
{
	AdmissionDecision decision{ AdmissionDecision::RejectedStage };
	uint32_t reservedSlots{ 0 };
	uint64_t ticket{ 0 };
};