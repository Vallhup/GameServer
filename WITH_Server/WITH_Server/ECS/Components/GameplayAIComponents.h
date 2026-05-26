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
	bool forceRetarget{ false };

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

struct AIActionRuntimeComp : Component
{
	std::vector<float> actionCooldownSec;
	std::vector<float> groupCooldownSec;

	float globalActionCooldownSec{ 0.0f };
	float movementLockSec{ 0.0f };

	uint32_t actionSequence{ 0 };

	uint32_t idleActionSequence{ 0 };
	double idleActionCooldownAcc{ 0.0 };

	AbilityId lastUsedAbilityId{ InvalidAbilityId };
};

struct AIMovementRuntimeComp : Component
{
	float strafeTimeLeftSec{ 0.0f };
	int strafeSign{ 1 };

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
