#pragma once

#include <cstdint>

enum class BodyPushability : uint8_t
{
	None = 0,
	Kinematic,
	Dynamic
};
