#pragma once

#include "Event.h"
#include "System.h"
#include <random>

struct AIState;
struct AIIntent;
struct AIThinkState;
struct AICombatTuning;

class AIThinkSystem : public System {
public:
	AIThinkSystem(WorldRuntime& rt, int p = 0);
	virtual ~AIThinkSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { };
	}

private:
	/*AttackType Think(Entity self, AIState* aiState);
	Entity FindTargetPlayer(Entity self, const AIState& aiState, const Transform& transform);
	Entity FindFarthestPlayer(Entity self, const Transform& selfTrans);*/


	bool EnsureTarget(Entity self, AIState& aiState, const Transform& selfTrans);
	bool IsValidTarget(Entity target) const;

	Entity FindClosestPlayer(Entity self, const Transform& selfTrans);
	
	void DecideIntent(
		Entity self,
		AIState& aiState,
		AIThinkState& thinkState,
		const Transform& selfTrans,
		const Transform& targetTrans,
		const AICombatTuning& tuning,
		AIIntent& outIntent
	);

	AttackType SelectAttack(
		Entity self,
		AIState& aiState,
		const Transform& selfTrans,
		const Transform& targetTrans,
		const AICombatTuning& tuning,
		double distSq
	);

	int CountPlayersInRange(Entity self, const Transform& center, double rangeSq) const;

	XMFLOAT3 MakeMoveDir(const Transform& from, const Transform& to);
	XMFLOAT3 MakeRetreatDir(const Transform& selfTrans, const Transform& targetTrans);
	XMFLOAT3 MakeStrafeDir(AIState& aiState, const Transform& selfTrans, const Transform& targetTrans);

	int RandomPercent();

	std::mt19937 _rng;
	std::uniform_int_distribution<int> _uid;
};

