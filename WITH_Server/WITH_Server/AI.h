#pragma once

#include "Entity.h"
#include "Component.h"

enum class AIIntentType : uint8_t
{
	None,
	Idle,
	Chase,
	Strafe,
	Retreate,
	Attack
};

struct AIState : public Component 
{
	Entity target;
	Entity lastAttacker;

	int patternsOnTarget;
	AttackType lastAttack{ AttackType::None };

	bool hasTarget{ false };

	bool strafeLeft{ true };
};

struct AIThinkState : public Component 
{
	double thinkAcc{ 0.0 };
	double thinkInterval{ 0.2 };

	double attackCooldownAcc{ 0.0 };
	double attackCooldown{ 5.0 };

	double retargetCooldownAcc{ 0.0 };
	double retargetCooldown{ 5.0 };

	double stuckAcc{ 0.0 };

	bool attackPending{ false };
};

struct AIIntent : public Component
{
	AIIntentType type{ AIIntentType::None };

	DirectX::XMFLOAT3 moveDir{ 0.0f, 0.0f, 0.0f };
	bool isRun{ false };

	AttackType attackType{ AttackType::None };
	Entity target;

	bool issued{ false };

	inline void Clear()
	{
		type		= AIIntentType::Idle;
		moveDir		= { 0.0f, 0.0f, 0.0f };
		isRun		= false;
		attackType	= AttackType::None;
		issued		= false;
	}
};

struct AICombatTuning : public Component
{
	double preferredMinDistance{ 1.0 };
	double preferredMaxDistance{ 6.0 };

	double attackDistance{ 15.0 };
	double chaseDistance{ 16.0 };
	double loseTargetDistance{ 25.0 };

	int meteorMinTargets{ 2 };
};