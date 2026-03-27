#pragma once

#include "WorldId.h"

class WorldIdAllocator {
public:
	explicit WorldIdAllocator(uint32_t reserve = 256);

	WorldId Allocate();
	void Free(WorldId worldId);

	bool IsAlive(WorldId worldId) const;

	void Clear();

private:
	std::vector<uint32_t> _gens;
	std::vector<uint32_t> _freeIds;
};

