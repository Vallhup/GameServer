#pragma once

#include "GameplayComponentPrerequisites.h"

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

	double timeSinceTargetLastSeen{ std::numeric_limits<double>::max() };

	uint64_t builtFrame{ 0 };
};

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
	double hardLeashRange{ 24.0 };
	double leashGaugeMax{ 100.0 };
	double leashDrainPerSec{ 20.0 };
	double hardLeashDrainPerSec{ 60.0 };
	double leashRecoverPerSec{ 35.0 };
	double returnHomeArriveRange{ 0.8 };
	double returnHomeReaggroLockSec{ 1.5 };
	double returnHpRegenPerSecRatio{ 0.08 };
	double idleActionCooldownSec{ 6.0 };
	int idleActionChancePercent{ 25 };
	double assistRange{ 6.0 };
};

struct AIBlackboardComp : Component
{
	Entity currentTarget{ Entity::Null() };
	Entity lastAttacker{ Entity::Null() };

	double timeSinceCurrentTargetSeen{ std::numeric_limits<double>::max() };

	bool forceRetarget{ false };

	XMFLOAT3 homePosition{ 0.0f, 0.0f, 0.0f };
	bool hasHomePosition{ false };
	bool returningHome{ false };
	double returnHomeLockoutAcc{ std::numeric_limits<double>::max() };
	double leashGauge{ 100.0 };
	double returnHpRegenAcc{ 0.0 };

	XMFLOAT3 lastKnownTargetPosition{ 0.0f, 0.0f, 0.0f };
	bool hasLastKnownTargetPosition{ false };

	ActionId lastUsedActionId{ ActionId::None };

	uint32_t combatActionSequence{ 0 };

	double idleActionCooldownAcc{ 0.0 };
	uint32_t idleActionSequence{ 0 };

	XMFLOAT3 pathDestination{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3 nextPathCorner{ 0.0f, 0.0f, 0.0f };
	bool hasPathCorner{ false };
	double pathRecomputeAcc{ 0.0 };
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

struct AIDecisionTuningComp : Component
{
	double decisionInterval{ 0.2 };
	double attackCooldown{ 1.8 };
	double reactDuration{ 0.5 };
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

struct AIReactionComp : Component
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

struct BossPhaseStateComp : Component
{
	uint8_t currentPhase{ 1 };
	float phase2ThresholdRatio{ 0.55f };
	uint32_t crossedThresholdMask{ 0 };
	bool transitionRequested{ false };
};

struct BossPatternRuntimeComp : Component
{
	static constexpr size_t kPatternCooldownSlotCount{ 5 };

	std::array<float, kPatternCooldownSlotCount> patternCooldownSec{};

	float phaseTransitionLockSec{ 0.0f };
	bool phaseTransitionActionPending{ false };

	float strafeTimeLeftSec{ 0.0f };
	int strafeSign{ 1 };
};

struct AICommandFrameComp : Component
{
	bool hasMove{ false };
	bool wantsRun{ false };
	XMFLOAT3 moveDir{ 0, 0, 0 };

	bool hasLook{ false };
	bool lockFacingToLookTarget{ false };
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
		lockFacingToLookTarget = false;
		sequence = 0;
	}

	inline void ClearAll()
	{
		*this = {};
	}
};
