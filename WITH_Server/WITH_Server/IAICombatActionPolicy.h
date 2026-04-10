#pragma once

#include "IDs.h"

struct AIContext;

// ── 공격 선택 결과 ────────────────────────────────────────────────────────────

struct CombatActionSelection
{
	bool     shouldAttack{ false };
	ActionId selectedActionId{ ActionId::None };
	float    directionX{ 0.0f };
	float    directionZ{ 0.0f };
};

// ── IAICombatActionPolicy ─────────────────────────────────────────────────────
//
// NormalAICombatState 에서 "어떤 공격 액션을 선택할지" 결정하는 정책 인터페이스.
// 하드코딩된 ActionId 를 제거하고, archetype 별로 교체 가능한 구조를 제공한다.
//
// 구현 책임:
//   - 공격 가능 여부 판단 (shouldAttack)
//   - 후보 액션 중 하나 선택 (selectedActionId)
//   - 공격 방향 계산 (directionX, directionZ)
//
// 구현 금지:
//   - 액션 자체의 쿨다운/조건 재정의 (ActionDef 의 영역)
//   - 상태 전환 요청 (IAIState 의 영역)

class IAICombatActionPolicy
{
public:
	virtual ~IAICombatActionPolicy() = default;

	virtual CombatActionSelection SelectAction(const AIContext& ctx) const = 0;
};
