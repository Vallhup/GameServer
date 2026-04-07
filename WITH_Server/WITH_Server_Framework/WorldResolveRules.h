#pragma once

#include <cstdint>

#include "WorldDef.h"
#include "WorldResolveKey.h"

enum class WorldResolveValidationResult : uint8_t
{
	Success,
	InvalidDef,
	InvalidInstanceKey
};

inline uint64_t NormalizeInstanceKey(const WorldDef& def, uint64_t rawInstanceKey)
{
	switch (def.topology.instanceType) {
	case WorldInstanceType::Persistent:
	{
		return 0;
	}
	case WorldInstanceType::Instanced:
	case WorldInstanceType::SessionScoped:
	{
		return rawInstanceKey;
	}
	default:
	{
		return 0;
	}
	}
}

inline WorldResolveKey MakeResolveKey(const WorldDef& def, uint64_t rawInstanceKey)
{
	return WorldResolveKey{
		def.id,
		NormalizeInstanceKey(def, rawInstanceKey)
	};
}

inline WorldResolveValidationResult ValidateResolveInput(const WorldDef& def, uint64_t rawInstanceKey)
{
	if (def.id == WorldDefId::None)
		return WorldResolveValidationResult::InvalidDef;

	switch (def.topology.instanceType) {
	case WorldInstanceType::Persistent:
	{
		return WorldResolveValidationResult::Success;
	}
	case WorldInstanceType::Instanced:
	case WorldInstanceType::SessionScoped:
	{
		return (rawInstanceKey != 0) ?
			WorldResolveValidationResult::Success :
			WorldResolveValidationResult::InvalidInstanceKey;
	}
	default:
	{
		return WorldResolveValidationResult::InvalidDef;
	}
	}
}