#pragma once

#include "IAIReactionPolicy.h"

// ── NormalReactionPolicy ──────────────────────────────────────────────────────
//
// 일반 잡몹 공용 반응 정책.
//
// 이벤트별 처리:
//   OnHitReceived → EnterReact
//   OnParried     → EnterReact + retargetAttacker (패리한 적을 우선 타겟)
//   OnGuardBroken → EnterReact
//   OnHpThreshold → Ignore (보스 전용 이벤트, 잡몹은 무시)

class NormalAIReactionPolicy final : public IAIReactionPolicy {
public:
	virtual ~NormalAIReactionPolicy() = default;

	virtual ReactionDecision Evaluate(
		const AIReactionEvent& topEvent,
		const AIContext&       ctx) const override;
};
