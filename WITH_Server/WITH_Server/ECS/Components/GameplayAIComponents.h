#pragma once

#include "GameplayComponentPrerequisites.h"
#include "../../GameplayContentIds.h"

struct AIControlledTag : TagComponent
{
};

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

	uint64_t builtFrame{ 0 };

	inline void Clear()
	{
		*this = {};
	}
};

struct AIBlackboardComp : Component
{
	// Target memory
	Entity currentTarget{ Entity::Null() };
	Entity lastAttacker{ Entity::Null() };
	Entity forcedTarget{ Entity::Null() };
	bool forceRetarget{ false };
	bool alternateTargetRetargetRequested{ false };

	Entity attackCountTarget{ Entity::Null() };
	uint16_t committedAttackCount{ 0 };

	double timeSinceCurrentTargetSeen{ std::numeric_limits<double>::max() };

	XMFLOAT3 lastKnownTargetPosition{ 0.0f, 0.0f, 0.0f };
	bool hasLastKnownTargetPosition{ false };

	// Home / leash / return-home
	XMFLOAT3 homePosition{ 0.0f, 0.0f, 0.0f };
	bool hasHomePosition{ false };

	bool returningHome{ false };
	double returnHomeLockoutAcc{ std::numeric_limits<double>::max() };

	double leashGauge{ 100.0 };
	double returnHpRegenAcc{ 0.0 };
};

enum class AIStateType : uint8_t
{
	Idle,
	Chase,
	Combat,
	Search,
	React,
	ReturnHome,

	Count
};

inline bool IsMonsterCombatAIState(AIStateType state) noexcept
{
	return state != AIStateType::Idle &&
		state != AIStateType::ReturnHome;
}

struct AIDecisionComp : Component
{
	AIStateType curState{ AIStateType::Idle };
	AIStateType prevState{ AIStateType::Idle };

	bool transitionRequested{ false };
	AIStateType requestedState{ AIStateType::Idle };

	double stateTime{ 0.0 };
	double globalDecisionAcc{ 0.0 };

	bool reactDurationOverrideActive{ false };
	double reactDurationOverrideSec{ 0.0f };

	bool enteredThisFrame{ true };

	void RequestTransition(AIStateType next) noexcept
	{
		transitionRequested = true;
		requestedState = next;
	}
};

enum class AIReactionEventType : uint8_t
{
	OnHitReceived = 0,
	OnParried = 1,
	OnGuardBroken = 2,
	OnHpThreshold = 3,
};

struct AIReactionEvent
{
	AIReactionEventType type{};
	Entity instigator{ Entity::Null() };
	int priority{ 0 };
	float floatPayload{ 0.0f };
};

struct AIReactionEventQueueComp : Component
{
	static constexpr int kMaxEventsPerFrame{ 4 };

	std::array<AIReactionEvent, kMaxEventsPerFrame> events{};
	int eventCount{ 0 };

	bool HasAnyEvent() const noexcept
	{
		return eventCount > 0;
	}

	const AIReactionEvent* TopPriorityEvent() const noexcept
	{
		if (eventCount == 0)
		{
			return nullptr;
		}

		const AIReactionEvent* top = &events[0];
		for (int i = 1; i < eventCount; ++i)
		{
			if (events[i].priority > top->priority)
			{
				top = &events[i];
			}
		}
		return top;
	}

	void PostEvent(AIReactionEvent evt) noexcept
	{
		if (eventCount < kMaxEventsPerFrame)
		{
			events[eventCount++] = evt;
			return;
		}

		int lowestIdx = 0;
		for (int i = 1; i < eventCount; ++i)
		{
			if (events[i].priority < events[lowestIdx].priority)
			{
				lowestIdx = i;
			}
		}

		if (evt.priority > events[lowestIdx].priority)
		{
			events[lowestIdx] = evt;
		}
	}

	void Clear() noexcept
	{
		eventCount = 0;
	}
};

struct AIPhaseRuntimeComp : Component
{
	static constexpr uint16_t kInvalidTransitionIndex =
		std::numeric_limits<uint16_t>::max();

	uint8_t currentPhase{ 1 };
	uint32_t crossedThresholdMask{ 0 };

	bool transitionRequested{ false };
	uint16_t pendingTransitionIndex{ kInvalidTransitionIndex };
};

enum class BossGimmickType : uint8_t
{
	None,
	PhaseTransitionObjects,
	FinalSafeZone
};

enum class BossGimmickStage : uint8_t
{
	None,
	Telegraph,
	Active,
	Resolve,
	Completed
};

struct BossGimmickStateComp : Component
{
	BossGimmickType activeType{ BossGimmickType::None };
	BossGimmickStage stage{ BossGimmickStage::None };
	uint32_t gimmickSeq{ 0 };

	float elapsedSec{ 0.0f };
	float stageElapsedSec{ 0.0f };
	float stageDurationSec{ 0.0f };

	bool blocksAI{ false };
	bool phaseTransitionGimmickRequested{ false };
	bool phaseTransitionGimmickCompleted{ false };
	bool finalGimmickRequested{ false };
	bool finalGimmickCompleted{ false };

	bool phaseTransitionObjectsSpawned{ false };
	bool phaseTransitionInstantKillResolved{ false };
	bool finalSafeZoneSpawned{ false };
	bool finalSafeZoneResolved{ false };
	// 사망(즉사) 타이머: 패턴 지속과 독립적으로 kGimmickLethalTimeSec 에 1회 판정.
	// gimmickLethalApplied = 판정 완료 여부, gimmickRunFailed = 그 판정에서 파훼 실패였는지.
	bool gimmickLethalApplied{ false };
	bool gimmickRunFailed{ false };
	std::vector<Entity> phaseTransitionObjectEntities;
	std::vector<Entity> phaseTransitionImmunePlayers;
	std::vector<Entity> finalSafeZoneEntities;

	bool IsActive() const noexcept
	{
		return activeType != BossGimmickType::None &&
			stage != BossGimmickStage::None &&
			stage != BossGimmickStage::Completed;
	}

	bool BlocksAI() const noexcept
	{
		return IsActive() && blocksAI;
	}

	void Request(BossGimmickType type) noexcept
	{
		switch (type) {
		case BossGimmickType::PhaseTransitionObjects:
			phaseTransitionGimmickRequested = true;
			break;
		case BossGimmickType::FinalSafeZone:
			finalGimmickRequested = true;
			break;
		default:
			break;
		}
	}

	void Begin(
		BossGimmickType type,
		BossGimmickStage initialStage,
		float durationSec,
		bool shouldBlockAI) noexcept
	{
		++gimmickSeq;
		activeType = type;
		stage = initialStage;
		elapsedSec = 0.0f;
		stageElapsedSec = 0.0f;
		stageDurationSec = std::max(0.0f, durationSec);
		blocksAI = shouldBlockAI;
		phaseTransitionObjectsSpawned = false;
		phaseTransitionInstantKillResolved = false;
		finalSafeZoneSpawned = false;
		finalSafeZoneResolved = false;
		gimmickLethalApplied = false;
		gimmickRunFailed = false;
		phaseTransitionObjectEntities.clear();
		phaseTransitionImmunePlayers.clear();
		finalSafeZoneEntities.clear();
	}

	void Complete() noexcept
	{
		switch (activeType) {
		case BossGimmickType::PhaseTransitionObjects:
			phaseTransitionGimmickCompleted = true;
			phaseTransitionGimmickRequested = false;
			break;
		case BossGimmickType::FinalSafeZone:
			finalGimmickCompleted = true;
			finalGimmickRequested = false;
			break;
		default:
			break;
		}

		activeType = BossGimmickType::None;
		stage = BossGimmickStage::Completed;
		elapsedSec = 0.0f;
		stageElapsedSec = 0.0f;
		stageDurationSec = 0.0f;
		blocksAI = false;
		phaseTransitionObjectsSpawned = false;
		phaseTransitionInstantKillResolved = false;
		finalSafeZoneSpawned = false;
		finalSafeZoneResolved = false;
		gimmickLethalApplied = false;
		gimmickRunFailed = false;
		phaseTransitionObjectEntities.clear();
		phaseTransitionImmunePlayers.clear();
		finalSafeZoneEntities.clear();
	}
};

struct AIActionRuntimeComp : Component
{
	std::vector<float> actionCooldownSec;
	std::vector<float> groupCooldownSec;

	float globalActionCooldownSec{ 0.0f };
	float movementLockSec{ 0.0f };

	uint32_t actionSequence{ 0 };
	uint16_t basicActionCountSinceEffect{ 0 };

	uint32_t idleActionSequence{ 0 };
	double idleActionCooldownAcc{ 0.0 };

	AbilityId lastUsedAbilityId{ InvalidAbilityId };
};

enum class AIMovementDistanceBand : uint8_t
{
	VeryClose,
	Close,
	Preferred,
	Mid,
	Far
};

struct AIMovementRuntimeComp : Component
{
	float strafeTimeLeftSec{ 0.0f };
	int strafeSign{ 1 };

	AIMovementDistanceBand combatDistanceBand{
		AIMovementDistanceBand::Far
	};
	bool hasCombatDistanceBand{ false };

	XMFLOAT3 pathDestination{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3 nextPathCorner{ 0.0f, 0.0f, 0.0f };
	bool hasPathCorner{ false };
	double pathRecomputeAcc{ 0.0 };
};

struct AIIntentFrameComp : Component
{
	bool hasMove{ false };
	bool wantsRun{ false };
	XMFLOAT3 moveDir{ 0, 0, 0 };

	bool hasLook{ false };
	bool lockFacingToLookTarget{ false };
	float moveYaw{ 0.0f };
	Entity target{ Entity::Null() };

	bool hasAbility{ false };
	AbilityId abilityId{ InvalidAbilityId };
	Entity abilityTarget{ Entity::Null() };
	float abilityDirX{ 0.0f };
	float abilityDirZ{ 0.0f };

	uint32_t sequence{ 0 };

	inline void ClearFrameTransient()
	{
		hasAbility = false;
		abilityId = InvalidAbilityId;
		abilityTarget = Entity::Null();
		abilityDirX = 0.0f;
		abilityDirZ = 0.0f;
		lockFacingToLookTarget = false;
		sequence = 0;
	}

	inline void ClearAll()
	{
		*this = {};
	}
};
