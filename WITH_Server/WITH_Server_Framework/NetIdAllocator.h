#pragma once

#include "NetId.h"

class NetIdAllocator {
public:
	explicit NetIdAllocator(uint32 reserve = 1024);

	NetId Allocate();
	void Free(NetId netId);

	bool IsAlive(NetId netId) const;

private:
	std::vector<uint16> _gens;
	std::vector<uint32> _freeIds;
};

