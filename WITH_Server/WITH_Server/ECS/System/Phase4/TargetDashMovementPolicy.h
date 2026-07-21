#pragma once

#include <algorithm>

namespace TargetDashMovementPolicy
{
	inline constexpr float kContactPadding = 0.05f;

	inline float ResolveStopDistance(
		float ownerRadius,
		float targetRadius,
		float stopDistanceOffset) noexcept
	{
		return
			std::max(ownerRadius, 0.0f) +
			std::max(targetRadius, 0.0f) +
			kContactPadding +
			std::max(stopDistanceOffset, 0.0f);
	}
}
