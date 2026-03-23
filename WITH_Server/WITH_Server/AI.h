#pragma once

#include "Entity.h"
#include "Component.h"

#include "Command.h"

struct AIPerceptionCache : Component
{
	Entity selectedTarget{ Entity::Null() };

	double distanceToTarget{ (std::numeric_limits<double>::max)() };
	double distanceToTargetSq{ (std::numeric_limits<double>::max)() };
	double targetForwardDot{ (std::numeric_limits<double>::min)() };

	bool hasTarget{ false };
	bool targetVisible{ false };
	bool targetInSightRange{ false };
	bool targetInAttackRange{ false };
	bool targetInFront{ false };

	uint32_t hostileInSightCount{ 0 };

	double timeSinceTargetLastSeen{ (std::numeric_limits<double>::max)() };

	uint64_t builtFrame{ 0 };
};

// TEMP : Arcetype이 같은 모든 Entity가 동일한 설정값을 들고 있는 것은 비효율적
//		  추후 Arcetype과 유사한 식별 System의 구현에 따라 Tuning Table 참조로 이전
struct AIPerceptionTuning : Component
{
	const double sightRange{ 12.0 };
	const double attackRange{ 2.5 };
	const double frontDotThreshold{ 0.2 };
	
	const double targetKeepBonus{ 4.0 };
	const double lastAttackerBonus{ 2.5 };
	const double frontBonus{ 1.0 };
	const double switchScoreMargin{ 3.0 };
	 
	const double loseSightGraceTime{ 1.2 };
	const double leashRange{ 18.0 };
	const double assistRange{ 6.0 };
};

struct AIBlackboard : Component
{
	Entity currentTarget{ Entity::Null() };
	Entity lastAttacker{ Entity::Null() };

	double timeSinceCurrentTargetSeen{ (std::numeric_limits<double>::max)() };

	bool forceRetarget{ false };

	XMFLOAT3 lastKnownTargetPosition{ 0, 0, 0 };
	bool hasLastKnownTargetPosition{ false };
};

enum class AIStateType : uint8_t
{
	Idle,	// 유효 타겟이 없을 때
	Chase,	// 타겟은 있지만 아직 공격 상태가 아닐 때
	Combat,	// 공격 사거리 진입 후 공격 / 회피 등 판단할 때
	Search,	// 타겟을 잃었지만 grace time 내에서 탐색할 때
	React	// 피격, 스턴 등 외부 이벤트 처리 상태
};

struct AIDecisionState : Component
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

	inline void RequestTransition(AIStateType next)
	{
		transitionRequested = true;
		requestedState = next;
	}
};

struct AIDecisionTuning : Component
{
	const double decisionInterval{ 0.2 };
	const double attackCooldown{ 1.2 };
};

struct AIReactionCache : Component
{
	Entity instigator{ Entity::Null() };
	bool gotHitThisFrame{ false };
	bool gotParriedThisFrame{ false };

	inline void Clear()
	{
		instigator = Entity::Null();
		gotHitThisFrame = false;
		gotParriedThisFrame = false;
	}

	inline bool GotReactionEvent() const
	{
		return gotHitThisFrame || gotParriedThisFrame;
	}
};

struct AIMotionIntent
{
	bool hasMove{ false };
	bool moveRun{ false };
	XMFLOAT3 moveDir{ 0, 0, 0 };

	bool hasLookTarget{ false };
	Entity lookTarget{ Entity::Null() };
};

struct AIActionIntent
{
	bool hasAction{ false };
	ActionType actionType{ ActionType::None };
	AttackType attackType{ AttackType::None };

	uint32_t sequence{ 0 };
};

struct AIIntent : Component
{
	/*bool clearMovement{ false };
	bool hasStrafe{ false };
	double strafeSign{ 0.0 };*/

	AIMotionIntent motion;
	AIActionIntent action;

	inline void ClearFrameTransient()
	{
		action = {};
	}

	void ClearAll()
	{
		motion = {};
		action = {};
	}
};

struct AIExecutionState : Component
{
	uint32_t lastConsumedActionSeq{ 0 };

	bool suppressMoveThisFrame{ false };
	bool suppressActionThisFrame{ false };

	CommandSource lastSource{ CommandSource::None };

	inline void ClearFrameTransient()
	{
		suppressMoveThisFrame = false;
		suppressActionThisFrame = false;
	}
};