#pragma once

#include "Entity.h"
#include "Component.h"

enum class AIMoveMode : uint8_t
{
	None,
	Hold,
	Walk,
	Run
};

enum class AICommandType : uint8_t
{
	None,
	Move,
	Attack
};

enum class AIBehaviorState : uint8_t
{
	Idle,
	Reposition,
	AttackWindow,
	CommitAttack,
	Recover
};

struct AIState : public Component
{
	Entity target;
	Entity lastAttacker;

	AttackType lastAttack{ AttackType::None };
	int patternsOnTarget{ 0 };
	bool strafeLeft{ true };
};

struct AISenseState : public Component
{
	bool hasTarget{ false };
	bool targetLost{ false };

	Entity target;

	double distToTarget{ 0.0 };
	double distSqToTarget{ 0.0 };

	XMFLOAT3 toTargetDir{ 0, 0, 0 };
	XMFLOAT3 awayFromTargetDir{ 0, 0, 0 };

	int nearbyPlayerCount{ 0 };

	bool targetInPreferredRange{ false };
	bool targetInAttackRange{ false };
	bool targetTooClose{ false };
	bool targetTooFar{ false };

	inline void Clear()
	{
		hasTarget = false;
		targetLost = false;

		target = {};

		distToTarget = 0.0;
		distSqToTarget = 0.0;

		toTargetDir = { 0, 0, 0 };
		awayFromTargetDir = { 0, 0, 0 };

		nearbyPlayerCount = 0;

		targetInPreferredRange = false;
		targetInAttackRange = false;
		targetTooClose = false;
		targetTooFar = false;
	}
};

struct AIThinkState : public Component 
{
	double thinkAcc{ 0.0 };
	double thinkInterval{ 0.2 };

	double attackCooldownAcc{ 0.0 };
	double attackCooldown{ 1.6 };

	double retargetCooldownAcc{ 0.0 };
	double retargetCooldown{ 5.0 };

	double stuckAcc{ 0.0 };
};

struct AICommand : public Component
{
	AICommandType type{ AICommandType::None };

	AIMoveMode moveMode{ AIMoveMode::None };
	XMFLOAT3 moveDir{ 0, 0, 0 };

	AttackType attackType{ AttackType::None };

	uint32_t serial{ 0 };

	inline void Clear()
	{
		type = AICommandType::None;
		moveMode = AIMoveMode::None;
		moveDir = { 0, 0, 0 };
		attackType = AttackType::None;
	}
};

struct AICombatTuning : public Component
{
	double preferredMinDistance{ 1.0 };
	double preferredMaxDistance{ 6.0 };

	double attackDistance{ 15.0 };
	double chaseDistance{ 16.0 };
	double loseTargetDistance{ 25.0 };

	double meteorCheckDistance{ 5.0 };
	int meteorMinTargets{ 2 };

	double nearCheckDistance{ 1.0 };
	double midCheckDistance{ 10.0 };
	double farCheckDistance{ 15.0 };

	double actionRequestTimeout{ 0.3 };
};

struct AIBehavior : public Component
{
	AIBehaviorState state{ AIBehaviorState::Idle };
	double stateTime{ 0.0 };
	double minStateDuration{ 0.0 };

	double moveBurstDuration{ 0.0 };
	XMFLOAT3 moveBurstDir{ 0, 0, 0 };
	bool moveBurstRun{ false };

	inline void ChangeBehavior(AIBehaviorState nextState, double minDuration)
	{
		if (state == nextState) return;
		state				= nextState;
		stateTime			= 0.0;
		minStateDuration	= minDuration;

		moveBurstDuration	= 0.0;
		moveBurstDir		= { 0, 0, 0 };
		moveBurstRun		= false;
	}
};

enum class AIActionRequestStatus : uint8_t
{
	None,
	Requested,
	Running,
	Finished,
	Rejected
};

struct AIActionRequestState : public Component
{
	uint32_t requestId{ 0 };
	AIActionRequestStatus status{ AIActionRequestStatus::None };

	ActionType requestedAction{ ActionType::None };
	AttackType requestedAttack{ AttackType::None };

	double elapsed{ 0.0 };

	bool requestIssued{ false };
	bool acceptedByActionSystem{ false };

	inline void Clear()
	{
		status = AIActionRequestStatus::None;
		requestedAction = ActionType::None;
		requestedAttack = AttackType::None;
		elapsed = 0.0;
		requestIssued = false;
		acceptedByActionSystem = false;
	}
};