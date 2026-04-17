#pragma once

#include "Entity.h"
#include "FrameworkRuntime.h"
#include "NetId.h"
#include "WorldId.h"

class ServerPlayerControlBinding final {
public:
	static bool AssignPlayerControlNetId(
		FrameworkRuntime& framework,
		WorldId worldId,
		Entity entity,
		NetId netId);
};
