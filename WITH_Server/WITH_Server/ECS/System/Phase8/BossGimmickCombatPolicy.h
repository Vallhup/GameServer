#pragma once

#include "../../Components/GameplayAIComponents.h"

namespace BossGimmickCombatPolicy
{
	inline bool ShouldIgnoreIncomingHpDamage(
		const BossGimmickStateComp& gimmick) noexcept
	{
		if (gimmick.IsActive())
		{
			return true;
		}

		if (gimmick.phaseTransitionGimmickRequested &&
			!gimmick.phaseTransitionGimmickCompleted)
		{
			return true;
		}

		return gimmick.finalGimmickRequested &&
			!gimmick.finalGimmickCompleted;
	}
}
