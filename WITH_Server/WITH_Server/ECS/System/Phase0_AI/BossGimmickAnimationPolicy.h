#pragma once

#include "../../Components/GameplayAIComponents.h"

namespace BossGimmickAnimationPolicy
{
	// 패턴(기믹) 지속 시간. 사망(즉사) 타이머와는 독립이며, 둘은 기믹 시작
	// 시점(Begin)에 동시에 시작한다. 사망 시점은 BossGimmickSystem.cpp 의
	// kGimmickLethalTimeSec 로 별도 관리한다.
	inline constexpr float kPhaseTransitionPatternDurationSec = 13.4f;
	inline constexpr float kFinalSafeZonePatternDurationSec = 12.0f;

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
		switch (type) {
		case BossGimmickType::PhaseTransitionObjects:
			return kPhaseTransitionPatternDurationSec;
		case BossGimmickType::FinalSafeZone:
			return kFinalSafeZonePatternDurationSec;
		default:
			return 0.0f;
		}
	}

	inline bool ShouldUseEntryAnimation(
		const BossGimmickStateComp& gimmick) noexcept
	{
		return gimmick.stage == BossGimmickStage::Telegraph &&
			EntryAnimationFor(gimmick.activeType) != AnimationId::None;
	}
}
