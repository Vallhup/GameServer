#pragma once

#include "../../Components/GameplayAIComponents.h"

namespace BossGimmickAnimationPolicy
{
	inline constexpr float kEntryAnimationDurationSec = 11.5f;

	inline AnimationId EntryAnimationFor(BossGimmickType type) noexcept
	{
		switch (type) {
		case BossGimmickType::PhaseTransitionObjects:
			return AnimationId::FinalBoss_50Percent;
		case BossGimmickType::FinalSafeZone:
			return AnimationId::FinalBoss_0Percent;
		default:
			return AnimationId::None;
		}
	}

	inline float EntryAnimationDurationFor(BossGimmickType type) noexcept
	{
		return EntryAnimationFor(type) != AnimationId::None
			? kEntryAnimationDurationSec
			: 0.0f;
	}

	inline bool ShouldUseEntryAnimation(
		const BossGimmickStateComp& gimmick) noexcept
	{
		return gimmick.stage == BossGimmickStage::Telegraph &&
			EntryAnimationFor(gimmick.activeType) != AnimationId::None;
	}
}
