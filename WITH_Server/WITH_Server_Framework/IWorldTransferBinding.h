#pragma once

#include <cstdint>

#include "Entity.h"

class IWorldTransferBinding {
public:
	virtual ~IWorldTransferBinding() = default;

	virtual bool TryResolveRootEntity(
		uint32_t sessionId,
		Entity& outEntity) const = 0;
};
