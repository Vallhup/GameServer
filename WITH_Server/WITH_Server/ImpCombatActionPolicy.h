#pragma once

#include "IAICombatActionPolicy.h"

// ── ImpCombatActionPolicy ─────────────────────────────────────────────────────
//
// Imp 잡몹 전용 공격 선택 정책.
//
// 선택 규칙:
//   - 쿨다운이 차지 않았으면 shouldAttack = false
//   - 직전 공격이 melee1 이면 30% 확률로 melee2 를 선택 (짧은 2타 연계)
//   - 그 외에는 melee1 을 기본으로 선택
//
// 확장 예정:
//   - 거리에 따른 돌진 공격 (melee3 ~ melee5) 선택
//   - 피격 횟수 기반 격노(enrage) 패턴 전환

class ImpCombatActionPolicy final : public IAICombatActionPolicy {
public:
	CombatActionSelection SelectAction(const AIContext& ctx) const override;
};
