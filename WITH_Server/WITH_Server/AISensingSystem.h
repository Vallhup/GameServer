#pragma once

#include "System.h"

struct AIState;
struct Transform;
struct AISenseState;
struct AICombatTuning;

class AISensingSystem : public System {
public:
	AISensingSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~AISensingSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return {  };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return {  };
	}

private:
	bool IsValidTarget(Entity target) const;

	Entity ResolveTarget(Entity self, AIState& aiState, const Transform& selfTrans);
	Entity FindClosestPlayer(Entity self, const Transform& selfTrans);

	void FillSense(
		Entity self,
		const AIState& aiState,
		const Transform& selfTrans,
		const AICombatTuning& tuning,
		AISenseState& outSense
	);

	int CountPlayersInRange(
		Entity self,
		const Transform& center,
		float rangeSq
	) const;

	static XMFLOAT3 MakeFlatDir(const Transform& from, const Transform& to);
};

// 1. 현재 AIState.target이 유효한지 검사
// 2. 유효하지 않으면 새 타겟 탐색
// 3. 타겟까지 거리/방향 계산
// 4. tuning 기준으로 범주화
// 5. 근처 플레이어 수 계산
// 6. 결과를 AISenseState에 기록
