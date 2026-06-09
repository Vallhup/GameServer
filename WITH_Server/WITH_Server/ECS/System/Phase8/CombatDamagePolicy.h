#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "../../../AbilityDef.h"

namespace CombatDamagePolicy
{
	inline int32_t ResolveGuardedHpDamage(
		int32_t rawDamage,
		int32_t defense,
		const AbilityGuardResponseDef* guardEffect) noexcept
	{
		if (rawDamage <= 0)
		{
			return 0;
		}

		float damageRatio = 1.0f;
		if (guardEffect != nullptr)
		{
			const float reducedDamageRatio =
				1.0f - std::clamp(
					guardEffect->damageReductionRatio,
					0.0f,
					1.0f);
			const float minimumChipDamageRatio =
				std::clamp(guardEffect->chipDamageRatio, 0.0f, 1.0f);
			damageRatio =
				std::max(reducedDamageRatio, minimumChipDamageRatio);
		}

		if (damageRatio <= 0.0f)
		{
			return 0;
		}

		const float mitigatedDamage =
			static_cast<float>(rawDamage) *
			damageRatio *
			100.0f /
			static_cast<float>(std::max(1, 100 + defense));
		return std::max(1, static_cast<int32_t>(std::lround(mitigatedDamage)));
	}
}
