#pragma once

#include "types.h"
#include "NetId.h"
#include "WorldId.h"

class IConnContext {
public:
	virtual ~IConnContext() = default;

	virtual bool TryGetWorld(uint32 connId, WorldId& out) const = 0;
	virtual bool TryGetOwnerPlayer(uint32 connId, NetId& out) const = 0;
};