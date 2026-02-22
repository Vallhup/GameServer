#pragma once

#include "WorldId.h"

class WorldIdAllocator {
public:
	explicit WorldIdAllocator(uint32 reserve = 256);

	WorldId Allocate();
	void Free(WorldId worldId);

	bool IsAlive(WorldId worldId) const;

	void Clear();

private:
	std::vector<uint32> _gens;
	std::vector<uint32> _freeIds;
};

