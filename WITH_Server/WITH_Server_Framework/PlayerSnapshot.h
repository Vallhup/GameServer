#pragma once

#include "NetId.h"

struct PlayerSnapshot
{
	NetId playerId;
	int hp;
	int stamina;
	std::vector<int> bufs;
};