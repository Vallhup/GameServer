#pragma once

#include "types.h"

enum class PacketType : uint8 {
	CS_LOGIN,
	CS_MOVE,
	CS_ATTACK,
	CS_DODGE,
	CS_GUARD,
	CS_PARRY,

	SC_LOGIN,
	SC_MOVE_OBJECT,
	SC_ADD,
	SC_REMOVE,
	SC_ANIMATION_CHANGE
};
