#pragma once

#include "types.h"

enum class PacketType : uint16 {
	CS_LOGIN,
	CS_MOVE,
	CS_ATTACK,
	CS_DODGE,

	SC_LOGIN,
	SC_MOVE_OBJECT,
	SC_ADD,
	SC_REMOVE
};
