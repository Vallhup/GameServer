#pragma once

#include <cstdint>

#include "Entity.h"
#include "NetId.h"

class IWorldTransferBinding {
public:
	virtual ~IWorldTransferBinding() = default;

	virtual bool TryResolveRootEntity(
		uint32_t sessionId,
		Entity& outEntity,
		NetId& outNetId) const = 0;
};
