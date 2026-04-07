#pragma once

#include "System.h"
#include "../../../IDs.h"
#include <DirectXMath.h>

using namespace DirectX;

struct ActionStateComp;
struct WorldTransformComp;
struct AIPerceptionComp;
struct AIPerceptionTuningComp;
struct AIBlackboardComp;
struct AIDecisionComp;
struct AIDecisionTuningComp;
struct AIReactionComp;
struct AICommandFrameComp;
struct SpawnTypeComp;

// AI Decision System
// - FSM (Idle/Chase/Combat/Search/React) 실행
// - AIReactionComp 소비 후 초기화
// - AICommandFrameComp에 이동/액션 의도 기록
// - CanIssueAction은 ActionDef 기반으로 판단
class AIDecisionSystem final : public System {
public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override;

private:
	// ActionDef 기반 액션 발행 가능 여부 판단
	static bool CanIssueAction(const ActionStateComp& state) noexcept;

	// FSM 실행 (전체 엔티티 처리 루프에서 호출)
	static void RunFSM(
		Entity entity,
		const WorldTransformComp& selfTr,
		const ActionStateComp& actionState,
		const AIPerceptionComp& perception,
		const AIPerceptionTuningComp& perceptionTuning,
		AIBlackboardComp& blackboard,
		AIDecisionComp& decision,
		const AIDecisionTuningComp& decisionTuning,
		AIReactionComp& reaction,
		AICommandFrameComp& frame,
		const ECSView& ecs,
		double dT) noexcept;

	static void ApplyPendingTransition(AIDecisionComp& decision) noexcept;

	// --- FSM 상태 핸들러 ---
	static void StateIdle_DecisionUpdate(
		const AIPerceptionComp& perception,
		AIDecisionComp& decision) noexcept;

	static void StateChase_FrameUpdate(
		const AIBlackboardComp& blackboard,
		AICommandFrameComp& frame,
		const ECSView& ecs) noexcept;

	static void StateChase_DecisionUpdate(
		const AIPerceptionComp& perception,
		const AIPerceptionTuningComp& perceptionTuning,
		AIDecisionComp& decision) noexcept;

	static void StateCombat_FrameUpdate(
		const AIBlackboardComp& blackboard,
		const AIPerceptionComp& perception,
		const AIPerceptionTuningComp& perceptionTuning,
		AICommandFrameComp& frame,
		const ECSView& ecs) noexcept;

	static void StateCombat_DecisionUpdate(
		const ActionStateComp& actionState,
		const AIPerceptionComp& perception,
		AIDecisionComp& decision,
		const AIDecisionTuningComp& tuning,
		AICommandFrameComp& frame,
		const SpawnTypeComp* spawnType) noexcept;

	static void StateSearch_FrameUpdate(
		const AIBlackboardComp& blackboard,
		const WorldTransformComp& selfTr,
		AICommandFrameComp& frame) noexcept;

	static void StateSearch_DecisionUpdate(
		const AIPerceptionComp& perception,
		const AIBlackboardComp& blackboard,
		const AIPerceptionTuningComp& perceptionTuning,
		AIDecisionComp& decision) noexcept;

	static void StateReact_DecisionUpdate(
		const AIPerceptionComp& perception,
		AIDecisionComp& decision) noexcept;

	// --- 이동 의도 빌더 ---
	static void BuildChaseIntent(
		const AIBlackboardComp& blackboard,
		const WorldTransformComp& selfTr,
		AICommandFrameComp& frame,
		const ECSView& ecs) noexcept;

	static void BuildCombatIntent(
		const AIBlackboardComp& blackboard,
		const WorldTransformComp& selfTr,
		const AIPerceptionComp& perception,
		const AIPerceptionTuningComp& perceptionTuning,
		AICommandFrameComp& frame,
		const ECSView& ecs) noexcept;

	static void BuildSearchIntent(
		const AIBlackboardComp& blackboard,
		const WorldTransformComp& selfTr,
		AICommandFrameComp& frame) noexcept;

	// 타겟 위치 취득 헬퍼
	static bool TryGetTargetPosition(
		const AIBlackboardComp& blackboard,
		const ECSView& ecs,
		XMFLOAT3& outPos) noexcept;

	// Combat 상태에서 AI 타입별 공격 ActionId 결정
	static ActionId ResolveAttackActionId(
		const SpawnTypeComp* spawnType) noexcept;

	static constexpr int kMaxDecisionStepsPerFrame = 4;

	static const SystemMeta kMeta;
};
