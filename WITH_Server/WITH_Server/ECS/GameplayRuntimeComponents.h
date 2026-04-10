#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "ActionDef.h"
#include "AICommand.h"
#include "AnimationDef.h"
#include "BodyCollisionTypes.h"
#include "PlayerCommand.h"
#include "RepComponent.h"
#include "Session.h"
#include "WorldContentIds.h"
#include "WorldDef.h"

enum class PlayerActionInputType : uint8_t
{
	None = 0,
	LightAttack,
	HeavyAttack,
	Dodge,
	Parry
};

struct PlayerControlIdentityComp : Component
{
	NetId netId{ NetId::Invalid() };
	SessionId ownerSessionId{ 0 };
};

struct PlayerMoveInputState
{
	float inputX{ 0.0f };
	float inputZ{ 0.0f };
	float cameraYawRad{ 0.0f };
	bool wantsRun{ false };
	uint64_t lastUpdatedFrame{ 0 };
};

struct PlayerGuardInputState
{
	bool isPressed{ false };
	uint64_t lastUpdatedFrame{ 0 };
};

struct ActorActionInputEvent
{
	// 플레이어 경로: PlayerActionInputType 를 request semantic 후보로 해석
	PlayerActionInputType type{ PlayerActionInputType::None };
	// AI 경로: ActionId 직접 지정 (즉시 전이가 아니라 request 후보로 처리)
	ActionId directActionId{ ActionId::None };
	float directionX{ 0.0f };
	float directionZ{ 0.0f };
	uint64_t requestedFrame{ 0 };
};

struct ActorInputComp : Component
{
	PlayerMoveInputState move;
	PlayerGuardInputState guard;
	ActorActionInputEvent action;
};

struct PendingDespawnTag : TagComponent
{
};

struct PendingWorldTransferTag : TagComponent
{
};

struct PendingWorldTransferComp : Component
{
	WorldDefId targetWorldDefId{ WorldDefId::None };
	uint64_t instanceKey{ 0 };
	SpawnPointId spawnPointId{ 0 };
	bool hasSpawnPointId{ false };
	bool allowFallback{ false };
	uint16_t sourceTriggerId{ 0 };
	uint64_t requestedFrameIndex{ 0 };
};

struct ActionInterruptEvent
{
	ActionInterruptCauseType causeType{
		ActionInterruptCauseType::OnHitReceived };
	Entity instigator{ Entity::Null() };
	uint64_t frameIndex{ 0 };
	int priority{ 0 };
};

struct ActionInterruptQueueComp : Component
{
	std::vector<ActionInterruptEvent> events;
};

struct PendingBuffApplyComp : Component
{
	BuffId buffId{ BuffId::None };
};

struct PendingBuffRemoveComp : Component
{
	BuffId buffId{ BuffId::None };
};

enum class LocomotionMode : uint8_t
{
	Idle = 0,
	Walk,
	Run,
	Turn
};

struct ActionStateComp : Component
{
	ActionId actionId{ ActionId::None };
	float elapsedSec{ 0.0f };
	uint32_t actionInstanceId{ 0 };
	float directionX{ 0.0f };
	float directionZ{ 0.0f };

	bool CanIssueAction() const noexcept
	{
		if (actionId == ActionId::None)
		{
			return true;
		}

		const ActionDef* def = FindActionDef(actionId);
		if (nullptr == def)
		{
			return true;
		}

		const float progress =
			(def->duration > 0.0f) ?
			std::clamp(elapsedSec / def->duration, 0.0f, 1.0f) :
			1.0f;

		// 1. 액션 자연 종료
		if (progress >= 1.0f)
		{
			return true;
		}

		// 2. AI Interruptible cancel window 진입 여부
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
			const float windowEnd	= cancel.windowEndNormalized.value_or(1.0f);
			if (progress >= windowStart && progress <= windowEnd)
			{
				return true;
			}
		}

		return false;
	}
};

struct LocomotionStateComp : Component
{
	LocomotionMode mode{ LocomotionMode::Idle };
	float desiredMoveDirX{ 0.0f };
	float desiredMoveDirZ{ 0.0f };
	float desiredFacingYawRad{ 0.0f };
	float facingYawRad{ 0.0f };
	float currentSpeed{ 0.0f };
	float locomotionAnimPhase01{ 0.0f };
	bool wasLocomotionMoving{ false };
};

struct PendingActionTimelineEvent
{
	EventType eventType{ EventType::PlayEffect };
	float timeNormalized{ 0.0f };
	std::optional<EventPayloadId> payloadId;
	TriggerConditionType conditionType{ TriggerConditionType::Always };
	ActionId sourceActionId{ ActionId::None };
	uint32_t sourceActionInstanceId{ 0 };
};

struct ActionTimelineAdvanceComp : Component
{
	ActionId actionId{ ActionId::None };
	uint32_t actionInstanceId{ 0 };
	float prevElapsedSec{ 0.0f };
	float currElapsedSec{ 0.0f };
	bool startedThisFrame{ false };
	std::vector<PendingActionTimelineEvent> events;
};

enum class AnimationPlaybackSource : uint8_t
{
	None = 0,
	Locomotion,
	Action
};

struct AnimationPlaybackStateComp : Component
{
	AnimationPlaybackSource source{ AnimationPlaybackSource::None };
	AnimationId animationId{ AnimationId::None };
	uint32_t boundActionInstanceId{ 0 };
	ActionId boundActionId{ ActionId::None };
	LocomotionMode boundLocomotionMode{ LocomotionMode::Idle };
	float playbackTimeSec{ 0.0f };
	float normalizedTime{ 0.0f };
	float playRate{ 1.0f };
	bool loop{ false };
	bool holdLastFrame{ false };
};

struct SampledAnimationPoseComp : Component
{
	AnimationId animationId{ AnimationId::None };
	uint16_t sampleFrameIndex{ 0 };
	std::vector<Capsule> localCapsules;
};

enum class SkeletalCombatColliderRoleMask : uint8_t
{
	None = 0,
	Hit = 1 << 0,
	Hurt = 1 << 1,
	Guard = 1 << 2,
	Parry = 1 << 3
};

struct SkeletalCombatCollider
{
	Capsule capsule;
	float radius{ 0.0f };
	uint8_t roleMask{ 0 };
};

struct SkeletalCombatColliderComp : Component
{
	std::vector<SkeletalCombatCollider> localColliders;
};

struct WorldTransformComp : Component
{
	XMFLOAT3 position{ 161.352478f, 48.737797f, 644.831543f };
	XMFLOAT4 rotation{ 0, 0, 0, 1 };
	XMFLOAT3 scale{ 1, 1, 1 };
};

struct LocomotionMoveDeltaComp : Component
{
	XMFLOAT3 deltaPosition{ 0.0f, 0.0f, 0.0f };
	float deltaYawRad{ 0.0f };
	bool hasDelta{ false };
};

struct ActionMoveDeltaComp : Component
{
	XMFLOAT3 deltaPosition{ 0.0f, 0.0f, 0.0f };
	float deltaYawRad{ 0.0f };
	bool hasDelta{ false };
};

struct ActionMoveRuntimeComp : Component
{
	uint32_t boundActionInstanceId{ 0 };
	float lockedDirX{ 0.0f };
	float lockedDirZ{ 0.0f };
	float lockedYawRad{ 0.0f };
	bool hasLockedDirection{ false };
};

struct PreCollisionTransformComp : Component
{
	XMFLOAT3 prevPosition{ 0.0f, 0.0f, 0.0f };
	XMFLOAT4 prevRotation{ 0.0f, 0.0f, 0.0f, 1.0f };

	XMFLOAT3 candidatePosition{ 0.0f, 0.0f, 0.0f };
	XMFLOAT4 candidateRotation{ 0.0f, 0.0f, 0.0f, 1.0f };

	bool movedThisFrame{ false };
	bool rotatedThisFrame{ false };
};

struct BodyCollisionShapeComp : Component
{
	float bodyRadiusXZ{ 0.5f };
	float bodyHeight{ 1.8f };
	bool blocksBodyOverlap{ true };
	bool useNavMeshConstraint{ true };
	BodyPushability pushability{ BodyPushability::Dynamic };
	float overlapYieldWeight{ 1.0f };
	float maxOverlapCorrectionPerFrameXZ{ 0.12f };
};

struct NavMeshAgentStateComp : Component
{
	uint64_t currentPolyRef{ 0 };
};

struct BodyCollisionResolveComp : Component
{
	XMFLOAT3 navResolvedPosition{ 0.0f, 0.0f, 0.0f };
	bool navMeshAdjusted{ false };
	bool navMeshFallbackNoProvider{ false };
	bool rejectedByNavMesh{ false };
	bool overlapAdjusted{ false };
};

struct PortalTriggerStateComp : Component
{
	uint16_t activeTriggerId{ 0 };
	bool wasInsideTrigger{ false };
};

struct CombatColliderActivationComp : Component
{
	uint32_t boundActionInstanceId{ 0 };
	ActionId boundActionId{ ActionId::None };
	bool hasAttackWindow{ false };
	bool hasParryWindow{ false };
	bool hasGuardWindow{ false };
	bool hasInvulnerabilityWindow{ false };
};

struct CombatHitDedupStateComp : Component
{
	uint32_t boundActionInstanceId{ 0 };
	std::vector<Entity> resolvedVictims;
};

enum class CombatReactionKind : uint8_t
{
	None = 0,
	HitReaction,
	GuardBreak,
	Knockdown
};

enum class CombatResolveResultType : uint8_t
{
	Hit = 0,
	Guard,
	Parry
};

struct PendingCombatInteractionRecord
{
	Entity sourceEntity{ Entity::Null() };
	ActionId sourceActionId{ ActionId::None };
	uint32_t sourceActionInstanceId{ 0 };
	uint16_t sourceAttackWindowIndex{ 0 };
	uint16_t sourceColliderIndex{ 0 };
	uint16_t targetColliderIndex{ 0 };
	CombatResolveResultType resultType{ CombatResolveResultType::Hit };
	AttackCombatEffectDef attackEffect{};
	std::optional<GuardCombatEffectDef> guardEffect;
	std::optional<ParryCombatEffectDef> parryEffect;
	float maxKnockbackDistance{ 0.0f };
	float maxHitStopSec{ 0.0f };
};

struct PendingCombatResultComp : Component
{
	std::vector<PendingCombatInteractionRecord> receivedInteractions;
	CombatReactionKind reactionKind{ CombatReactionKind::None };
	Entity reactionSource{ Entity::Null() };
	bool guardSucceededThisFrame{ false };
	bool parrySucceededThisFrame{ false };
	bool wasHitThisFrame{ false };
	bool parriedByAnyVictimThisFrame{ false };
	bool hitAnyVictimThisFrame{ false };
	std::optional<BuffId> pendingParryBuffId;
};

struct CombatStatStateComp : Component
{
	int32_t currentHp{ 0 };
	int32_t maxHp{ 0 };
	int32_t currentStamina{ 0 };
	int32_t maxStamina{ 0 };
	int32_t currentPoise{ 0 };
	int32_t maxPoise{ 0 };
	int32_t attackPower{ 0 };
	int32_t defense{ 0 };
	float attackSpeed{ 1.0f };
	float moveSpeed{ 2.5f };
};

struct ActiveBuffRuntimeEntry
{
	BuffId buffId{ BuffId::None };
	float remainingDurationSec{ 0.0f };
	uint32_t stackCount{ 0 };
	uint64_t appliedOrder{ 0 };
};

struct BuffRuntimeStateComp : Component
{
	std::vector<ActiveBuffRuntimeEntry> activeBuffs;
};

struct PendingProjectileSpawnRequest
{
	Entity sourceEntity{ Entity::Null() };
	ActionId sourceActionId{ ActionId::None };
	uint32_t sourceActionInstanceId{ 0 };
	std::optional<EventPayloadId> payloadId;
};

struct PendingProjectileSpawnComp : Component
{
	std::vector<PendingProjectileSpawnRequest> requests;
};

struct PendingActionPresentationEvent
{
	Entity sourceEntity{ Entity::Null() };
	ActionId sourceActionId{ ActionId::None };
	uint32_t sourceActionInstanceId{ 0 };
	EventType eventType{ EventType::PlayEffect };
	std::optional<EventPayloadId> payloadId;
};

struct PendingActionPresentationEventComp : Component
{
	std::vector<PendingActionPresentationEvent> events;
};

struct PortalTriggerDef
{
	uint16_t triggerId{ 0 };
	XMFLOAT3 center{ 0.0f, 0.0f, 0.0f };
	float radiusXZ{ 0.0f };
	WorldDefId targetWorldDefId{ WorldDefId::None };
	uint64_t instanceKey{ 0 };
	SpawnPointId spawnPointId{ 0 };
	bool hasSpawnPointId{ false };
	bool allowFallback{ false };
};

struct ReplicationStatsComp : Component
{
	uint64_t droppedUnsupportedCommandTypeCount{ 0 };
	uint64_t droppedInvalidPayloadCount{ 0 };
	uint64_t droppedTargetMissingCount{ 0 };
	uint64_t ownerMismatchCount{ 0 };
	uint64_t deferredPotionEventCount{ 0 };
	uint64_t missingAnimationRegistryCount{ 0 };
	uint64_t missingWorldTransferPayloadCount{ 0 };
	uint64_t replicationTodoSkippedCount{ 0 };
};

// ============================================================
// AI 컴포넌트
// ============================================================

// AI 엔티티 식별 태그
struct AIControlledTag : TagComponent {};

// AI 인지 캐시 (매 프레임 재계산되는 read-only 스냅샷)
struct AIPerceptionComp : Component
{
	Entity selectedTarget{ Entity::Null() };

	double distanceToTarget{ std::numeric_limits<double>::max() };
	double distanceToTargetSq{ std::numeric_limits<double>::max() };
	double targetForwardDot{ std::numeric_limits<double>::lowest() };

	bool hasTarget{ false };
	bool targetVisible{ false };
	bool targetInSightRange{ false };
	bool targetInAttackRange{ false };
	bool targetInFront{ false };

	uint32_t hostileInSightCount{ 0 };

	double timeSinceTargetLastSeen{ std::numeric_limits<double>::max() };

	uint64_t builtFrame{ 0 };
};

// AI 인지 튜닝 파라미터
// TEMP : Arcetype이 같은 모든 Entity가 동일한 설정값을 들고 있는 것은 비효율적
//		  추후 Arcetype과 유사한 식별 System의 구현에 따라 Tuning Table 참조로 이전
struct AIPerceptionTuningComp : Component
{
	double sightRange{ 12.0 };
	double attackRange{ 2.0 };
	double frontDotThreshold{ 0.2 };
	
	double targetKeepBonus{ 4.0 };
	double lastAttackerBonus{ 2.5 };
	double frontBonus{ 1.0 };
	double switchScoreMargin{ 3.0 };
	
	double loseSightGraceTime{ 1.2 };
	double leashRange{ 18.0 };
	double assistRange{ 6.0 };
};

// AI 블랙보드 (AI 기억 공간)
struct AIBlackboardComp : Component
{
	Entity currentTarget{ Entity::Null() };
	Entity lastAttacker{ Entity::Null() };

	double timeSinceCurrentTargetSeen{ std::numeric_limits<double>::max() };

	bool forceRetarget{ false };

	XMFLOAT3 lastKnownTargetPosition{ 0.0f, 0.0f, 0.0f };
	bool hasLastKnownTargetPosition{ false };

	// 마지막으로 실행한 액션 (IAICombatActionPolicy 에서 콤보 다양성 판단에 활용)
	ActionId lastUsedActionId{ ActionId::None };
};

enum class AIStateType : uint8_t
{
	Idle,	// 유효 타겟이 없을 때
	Chase,	// 타겟은 있지만 아직 공격 상태가 아닐 때
	Combat,	// 공격 사거리 진입 후 공격 / 회피 등 판단할 때
	Search,	// 타겟을 잃었지만 grace time 내에서 탐색할 때
	React	// 피격, 스턴 등 외부 이벤트 처리 상태
};

// AI FSM 의사결정 상태
struct AIDecisionComp : Component
{
	AIStateType curState{ AIStateType::Idle };
	AIStateType prevState{ AIStateType::Idle };

	bool transitionRequested{ false };
	AIStateType requestedState{ AIStateType::Idle };

	double stateTime{ 0.0 };
	double globalDecisionAcc{ 0.0 };

	double attackCooldownAcc{ 0.0 };
	double repathCooldownAcc{ 0.0 };

	bool enteredThisFrame{ true };

	void RequestTransition(AIStateType next) noexcept
	{
		transitionRequested = true;
		requestedState = next;
	}
};

// AI 의사결정 튜닝 파라미터
struct AIDecisionTuningComp : Component
{
	double decisionInterval{ 0.2 };
	double attackCooldown{ 1.8 };

	// React 상태 최소 체류 시간. IAIReactionPolicy 가 결과를 반환한 후
	// 이 시간이 지나야 다음 상태로 전환 시도한다.
	double reactDuration{ 0.5 };
};

// AI 반응 이벤트 타입
// 우선순위는 숫자가 클수록 높다 (AIReactionEvent::priority 필드로 별도 관리)
enum class AIReactionEventType : uint8_t
{
	OnHitReceived = 0, // 일반 피격
	OnParried     = 1, // 공격이 패리됨
	OnGuardBroken = 2, // 가드 브레이크
	OnHpThreshold = 3, // HP 임계값 도달 (보스 기믹용)
};

// AI 반응 이벤트 (Phase 8에서 PostEvent, AIDecisionSystem에서 소비 후 Clear)
struct AIReactionEvent
{
	AIReactionEventType type{};
	Entity              instigator{ Entity::Null() };
	int                 priority{ 0 };
	float               floatPayload{ 0.0f }; // OnHpThreshold 시 HP 비율 등 부가 데이터
};

// AI 반응 이벤트 큐 (고정 크기 배열 — heap 할당 없음)
struct AIReactionComp : Component
{
	static constexpr int kMaxEventsPerFrame{ 4 };

	std::array<AIReactionEvent, kMaxEventsPerFrame> events{};
	int eventCount{ 0 };

	bool HasAnyEvent() const noexcept
	{
		return eventCount > 0;
	}

	// 우선순위가 가장 높은 이벤트를 반환 (없으면 nullptr)
	const AIReactionEvent* TopPriorityEvent() const noexcept
	{
		if (eventCount == 0)
			return nullptr;

		const AIReactionEvent* top = &events[0];
		for (int i = 1; i < eventCount; ++i)
		{
			if (events[i].priority > top->priority)
				top = &events[i];
		}
		return top;
	}

	// 이벤트 등록 (큐가 가득 찼으면 우선순위가 낮은 이벤트와 교체)
	void PostEvent(AIReactionEvent evt) noexcept
	{
		if (eventCount < kMaxEventsPerFrame)
		{
			events[eventCount++] = evt;
			return;
		}

		// 큐 포화 시 우선순위가 가장 낮은 슬롯과 교체
		int lowestIdx = 0;
		for (int i = 1; i < eventCount; ++i)
		{
			if (events[i].priority < events[lowestIdx].priority)
				lowestIdx = i;
		}
		if (evt.priority > events[lowestIdx].priority)
			events[lowestIdx] = evt;
	}

	void Clear() noexcept
	{
		eventCount = 0;
	}
};

// AI 프레임 단위 명령 출력 (transient - 매 프레임 초기화)
struct AICommandFrameComp : Component
{
	bool hasMove{ false };
	bool wantsRun{ false };
	XMFLOAT3 moveDir{ 0, 0, 0 };

	bool hasLook{ false };
	float moveYaw{ 0.0f };
	Entity target{ Entity::Null() };

	bool hasAction{ false };
	ActionId actionId{ ActionId::None };
	float actionDirX{ 0.0f };
	float actionDirZ{ 0.0f };

	uint32_t sequence{ 0 };

	inline void ClearFrameTransient()
	{
		hasAction = false;
		actionId = ActionId::None;
		actionDirX = 0.0f;
		actionDirZ = 0.0f;
		sequence = 0;
	}

	inline void ClearAll()
	{
		*this = {};
	}
};
