#include "pch.h"
#include "AIDecisionSystem.h"

#include <DirectXMath.h>

#include "../../GameplayRuntimeComponents.h"
#include "../GameplaySystemUtil.h"
#include "RepComponent.h"

using namespace GameplaySystemUtil;

const SystemMeta AIDecisionSystem::kMeta =
	MakeSystemMeta<AIDecisionSystem>("AIDecisionSystem");

// ============================================================
// CanIssueAction: ActionDef 기반 액션 발행 가능 여부
// ============================================================
bool AIDecisionSystem::CanIssueAction(const ActionStateComp& state) noexcept
{
	if (state.actionId == ActionId::None)
	{
		return true;
	}

	const ActionDef* def = FindActionDef(state.actionId);
	if (def == nullptr)
	{
		return true;
	}

	const float progress = (def->duration > 0.0f)
		? std::clamp(state.elapsedSec / def->duration, 0.0f, 1.0f)
		: 1.0f;

	// 1. 액션 자연 종료
	if (progress >= 1.0f)
	{
		return true;
	}

	// 2. aiInterruptible cancel window 진입 여부
	for (const ActionCancelRule& cancel : def->transitionRule.cancelRules)
	{
		if (!cancel.aiInterruptible)
		{
			continue;
		}

		if (cancel.windowPolicy == ActionWindowPolicy::Always)
		{
			return true;
		}

		const float windowStart = cancel.windowStartNormalized.value_or(0.0f);
		const float windowEnd   = cancel.windowEndNormalized.value_or(1.0f);
		if (progress >= windowStart && progress <= windowEnd)
		{
			return true;
		}
	}

	return false;
}

// ============================================================
// 타겟 위치 취득
// ============================================================
bool AIDecisionSystem::TryGetTargetPosition(
	const AIBlackboardComp& blackboard,
	const ECSView& ecs,
	XMFLOAT3& outPos) noexcept
{
	if (blackboard.currentTarget.IsNull())
	{
		return false;
	}

	const WorldTransformComp* targetTr =
		ecs.GetComponent<WorldTransformComp>(blackboard.currentTarget);
	if (targetTr == nullptr)
	{
		return false;
	}

	outPos = targetTr->position;
	return true;
}

// ============================================================
// 공격 ActionId 결정 (AI 타입별)
// ============================================================
ActionId AIDecisionSystem::ResolveAttackActionId(
	const SpawnTypeComp* spawnType) noexcept
{
	if (spawnType == nullptr)
	{
		return ActionId::None;
	}

	// AI 캐릭터별 기본 공격 액션 (추후 ActionDef 검색 방식으로 확장 가능)
	switch (spawnType->characterId) {
	case CharacterId::Imp:
		return ActionId::Imp_melee1;
	case CharacterId::FinalBoss:
		return ActionId::FinalBoss_Slash;
	default:
		return ActionId::None;
	}
}

// ============================================================
// 이동 의도 빌더 (MovementPolicy 역할)
// ============================================================
void AIDecisionSystem::BuildChaseIntent(
	const AIBlackboardComp& blackboard,
	const WorldTransformComp& selfTr,
	AICommandFrameComp& frame,
	const ECSView& ecs) noexcept
{
	XMFLOAT3 targetPos{};
	if (!TryGetTargetPosition(blackboard, ecs, targetPos))
	{
		return;
	}

	const float dx = targetPos.x - selfTr.position.x;
	const float dz = targetPos.z - selfTr.position.z;
	const float len = std::sqrt(dx * dx + dz * dz);
	if (len < 1e-5f)
	{
		return;
	}

	frame.hasMove  = true;
	frame.wantsRun = true;
	frame.moveX    = dx / len;
	frame.moveZ    = dz / len;
	frame.moveYaw  = selfTr.yawRad;
}

void AIDecisionSystem::BuildCombatIntent(
	const AIBlackboardComp& blackboard,
	const WorldTransformComp& selfTr,
	const AIPerceptionComp& perception,
	const AIPerceptionTuningComp& perceptionTuning,
	AICommandFrameComp& frame,
	const ECSView& ecs) noexcept
{
	XMFLOAT3 targetPos{};
	if (!TryGetTargetPosition(blackboard, ecs, targetPos))
	{
		return;
	}

	const float dx = targetPos.x - selfTr.position.x;
	const float dz = targetPos.z - selfTr.position.z;
	const float len = std::sqrt(dx * dx + dz * dz);
	if (len < 1e-5f)
	{
		return;
	}

	const float toX = dx / len;
	const float toZ = dz / len;

	const double dist    = perception.distanceToTarget;
	const double desired = perceptionTuning.attackRange * 0.9;
	const double tooClose = desired * 0.65;
	const double tooFar   = desired * 1.10;

	frame.hasMove  = true;
	frame.wantsRun = false;
	frame.moveYaw  = selfTr.yawRad;

	if (dist > tooFar)
	{
		// 접근
		frame.moveX = toX;
		frame.moveZ = toZ;
	}
	else if (dist < tooClose)
	{
		// 후퇴
		frame.moveX = -toX;
		frame.moveZ = -toZ;
	}
	else
	{
		// 거리 유지: 이동 없음
		frame.hasMove = false;
	}
}

void AIDecisionSystem::BuildSearchIntent(
	const AIBlackboardComp& blackboard,
	const WorldTransformComp& selfTr,
	AICommandFrameComp& frame) noexcept
{
	if (!blackboard.hasLastKnownTargetPosition)
	{
		return;
	}

	const float dx = blackboard.lastKnownTargetPosition.x - selfTr.position.x;
	const float dz = blackboard.lastKnownTargetPosition.z - selfTr.position.z;
	const float lenSq = dx * dx + dz * dz;
	if (lenSq < 1e-5f)
	{
		return;
	}

	const float len = std::sqrt(lenSq);
	frame.hasMove  = true;
	frame.wantsRun = false;
	frame.moveX    = dx / len;
	frame.moveZ    = dz / len;
	frame.moveYaw  = selfTr.yawRad;
}

// ============================================================
// FSM 상태별 핸들러
// ============================================================
void AIDecisionSystem::StateIdle_DecisionUpdate(
	const AIPerceptionComp& perception,
	AIDecisionComp& decision) noexcept
{
	if (perception.hasTarget)
	{
		decision.RequestTransition(AIStateType::Chase);
	}
}

void AIDecisionSystem::StateChase_FrameUpdate(
	const AIBlackboardComp& blackboard,
	AICommandFrameComp& frame,
	const ECSView& ecs) noexcept
{
	// lookTarget은 ActionMoveRuntime이나 별도 시스템에서 처리
	// 현재는 이동 방향으로 facing 처리 (Phase 2 로코모션 시스템이 담당)
	(void)blackboard;
	(void)frame;
	(void)ecs;
}

void AIDecisionSystem::StateChase_DecisionUpdate(
	const AIPerceptionComp& perception,
	const AIPerceptionTuningComp& perceptionTuning,
	AIDecisionComp& decision) noexcept
{
	if (!perception.hasTarget)
	{
		decision.RequestTransition(AIStateType::Search);
		return;
	}

	if (perception.targetInAttackRange && perception.targetInFront)
	{
		decision.RequestTransition(AIStateType::Combat);
	}
	(void)perceptionTuning;
}

void AIDecisionSystem::StateCombat_FrameUpdate(
	const AIBlackboardComp& blackboard,
	const AIPerceptionComp& perception,
	const AIPerceptionTuningComp& perceptionTuning,
	AICommandFrameComp& frame,
	const ECSView& ecs) noexcept
{
	(void)perception;
	(void)perceptionTuning;
	(void)ecs;
	(void)blackboard;
	(void)frame;
}

void AIDecisionSystem::StateCombat_DecisionUpdate(
	const ActionStateComp& actionState,
	const AIPerceptionComp& perception,
	AIDecisionComp& decision,
	const AIDecisionTuningComp& tuning,
	AICommandFrameComp& frame,
	const SpawnTypeComp* spawnType) noexcept
{
	if (!perception.hasTarget)
	{
		decision.RequestTransition(AIStateType::Search);
		return;
	}

	if (!perception.targetInAttackRange)
	{
		decision.RequestTransition(AIStateType::Chase);
		return;
	}

	if (!CanIssueAction(actionState))
	{
		return;
	}

	if (decision.attackCooldownAcc < tuning.attackCooldown)
	{
		return;
	}

	const ActionId attackId = ResolveAttackActionId(spawnType);
	if (attackId == ActionId::None)
	{
		return;
	}

	frame.hasAction   = true;
	frame.actionId    = attackId;
	frame.actionDirX  = static_cast<float>(
		std::sin(static_cast<double>(perception.targetForwardDot)));
	frame.actionDirZ  = 1.0f;

	decision.attackCooldownAcc = 0.0;
}

void AIDecisionSystem::StateSearch_FrameUpdate(
	const AIBlackboardComp& blackboard,
	const WorldTransformComp& selfTr,
	AICommandFrameComp& frame) noexcept
{
	BuildSearchIntent(blackboard, selfTr, frame);
}

void AIDecisionSystem::StateSearch_DecisionUpdate(
	const AIPerceptionComp& perception,
	const AIBlackboardComp& blackboard,
	const AIPerceptionTuningComp& perceptionTuning,
	AIDecisionComp& decision) noexcept
{
	if (perception.hasTarget)
	{
		decision.RequestTransition(AIStateType::Chase);
		return;
	}

	if (blackboard.timeSinceCurrentTargetSeen > perceptionTuning.loseSightGraceTime)
	{
		decision.RequestTransition(AIStateType::Idle);
	}
}

void AIDecisionSystem::StateReact_DecisionUpdate(
	const AIPerceptionComp& perception,
	AIDecisionComp& decision) noexcept
{
	if (decision.stateTime >= 0.1)
	{
		if (perception.hasTarget)
		{
			decision.RequestTransition(AIStateType::Chase);
		}
		else
		{
			decision.RequestTransition(AIStateType::Idle);
		}
	}
}

// ============================================================
// 상태 전환 적용
// ============================================================
void AIDecisionSystem::ApplyPendingTransition(AIDecisionComp& decision) noexcept
{
	if (!decision.transitionRequested)
	{
		return;
	}

	const AIStateType cur  = decision.curState;
	const AIStateType next = decision.requestedState;

	decision.transitionRequested = false;

	if (cur == next)
	{
		return;
	}

	decision.prevState  = cur;
	decision.curState   = next;
	decision.stateTime  = 0.0;
	decision.enteredThisFrame = true;
}

// ============================================================
// RunFSM 메인 로직
// ============================================================
void AIDecisionSystem::RunFSM(
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
	double dT) noexcept
{
	frame.Clear();

	const SpawnTypeComp* spawnType = ecs.GetComponent<SpawnTypeComp>(entity);

	// 1. 반응 이벤트 우선 처리
	if (reaction.GotReactionEvent())
	{
		if (!reaction.instigator.IsNull())
		{
			blackboard.lastAttacker   = reaction.instigator;
			blackboard.forceRetarget  = true;
		}
		reaction.Clear();
		decision.RequestTransition(AIStateType::React);
		ApplyPendingTransition(decision);
		return;
	}
	reaction.Clear();

	// 2. 상태 진입 처리 (Enter 호출 대신 flag 초기화)
	if (decision.enteredThisFrame)
	{
		frame.Clear();
		decision.enteredThisFrame = false;
	}

	// 3. FrameUpdate (이동 의도 빌드)
	switch (decision.curState) {
	case AIStateType::Chase:
		BuildChaseIntent(blackboard, selfTr, frame, ecs);
		break;
	case AIStateType::Combat:
		BuildCombatIntent(
			blackboard, selfTr, perception, perceptionTuning, frame, ecs);
		break;
	case AIStateType::Search:
		BuildSearchIntent(blackboard, selfTr, frame);
		break;
	default:
		break;
	}

	// 4. DecisionUpdate (인터벌 기반)
	decision.stateTime         += dT;
	decision.globalDecisionAcc += dT;
	decision.attackCooldownAcc += dT;

	int steps = 0;
	while (decision.globalDecisionAcc >= decisionTuning.decisionInterval &&
		steps < kMaxDecisionStepsPerFrame)
	{
		decision.globalDecisionAcc -= decisionTuning.decisionInterval;

		switch (decision.curState) {
		case AIStateType::Idle:
			StateIdle_DecisionUpdate(perception, decision);
			break;
		case AIStateType::Chase:
			StateChase_DecisionUpdate(perception, perceptionTuning, decision);
			break;
		case AIStateType::Combat:
			StateCombat_DecisionUpdate(
				actionState, perception, decision, decisionTuning, frame, spawnType);
			break;
		case AIStateType::Search:
			StateSearch_DecisionUpdate(
				perception, blackboard, perceptionTuning, decision);
			break;
		case AIStateType::React:
			StateReact_DecisionUpdate(perception, decision);
			break;
		}

		ApplyPendingTransition(decision);
		++steps;
	}
}

// ============================================================
// Execute
// ============================================================
void AIDecisionSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, _, selfTr, actionState, perception, perceptionTuning,
		blackboard, decision, decisionTuning, reaction, frame] :
		ctx.ecs.View<
			AIControlledTag,
			WorldTransformComp,
			ActionStateComp,
			AIPerceptionComp,
			AIPerceptionTuningComp,
			AIBlackboardComp,
			AIDecisionComp,
			AIDecisionTuningComp,
			AIReactionComp,
			AICommandFrameComp>())
	{
		RunFSM(
			entity,
			selfTr,
			actionState,
			perception,
			perceptionTuning,
			blackboard,
			decision,
			decisionTuning,
			reaction,
			frame,
			ctx.ecs,
			ctx.dtSec);
	}
}

const SystemMeta& AIDecisionSystem::Meta() const
{
	return kMeta;
}
