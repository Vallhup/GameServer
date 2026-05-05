#pragma once

#include "ECS/GameplayRuntimeComponents.h"
#include "GameplayContentIds.h"

struct AIContext;

// ── 반응 전술 결과 ────────────────────────────────────────────────────────────

enum class ReactionTacticalOutcome : uint8_t
{
	Ignore,        // 이벤트 무시 — React 상태 전환 없음
	EnterReact,    // React 상태로 전환
	ForceRetarget, // 타겟 재평가 강제 (React 전환 없이 blackboard 만 수정)
};

struct ReactionDecision
{
	ReactionTacticalOutcome outcome{ ReactionTacticalOutcome::Ignore };

	// true 이면 instigator 를 즉시 currentTarget 으로 설정
	bool retargetAttacker{ false };

	// None 이 아니면 Enter 시 이 액션을 즉시 발행 (현재 미사용, 확장용)
	AbilityId reactAbilityOverride{ InvalidAbilityId };
};

// ── IAIReactionPolicy ─────────────────────────────────────────────────────────
//
// AIReactionComp 의 TopPriorityEvent 를 전술적으로 해석하는 정책 인터페이스.
// NormalAIReactState 진입 직전, AIDecisionSystem 의 RunFSM 에서 호출된다.
//
// 구현 책임:
//   - 이벤트 타입과 우선순위를 보고 전술적 반응 결정
//   - 타겟 재지정 여부 반환
//
// 구현 금지:
//   - 상태 전환 직접 호출 (AIContext::decision 직접 수정 금지)
//   - 피격/패리 결과 계산 (CommitCombatResultSystem 의 영역)

class IAIReactionPolicy {
public:
	virtual ~IAIReactionPolicy() = default;

	virtual ReactionDecision Evaluate(
		const AIReactionEvent& topEvent,
		const AIContext&       ctx) const = 0;
};
