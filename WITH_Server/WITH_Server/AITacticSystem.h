#pragma once

#include "System.h"

struct AIState;
struct AIBehavior;
struct ActionState;
struct AISenseState;
struct AIThinkState;
struct AIActionRequestState;

class AITacticSystem : public System {
public:
	AITacticSystem(WorldRuntime& rt, int p = 0);
	virtual ~AITacticSystem() = default;

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
	void UpdateTactic(
		const ActionState& actionState,
		AIState& aiState,
		const AISenseState& sense,
		AIThinkState& think,
		AIBehavior& behavior,
		AIActionRequestState& req
	);

	void EnterReposition(
		AIState& aiState,
		const AISenseState& sense,
		AIBehavior& behavior
	);

	XMFLOAT3 MakeStrafeDir(bool strafeLeft, const XMFLOAT3& toTargetDir) const;

	int RandomPercent();
	double RandRange(double minV, double maxV);


	std::mt19937 _rng;
	std::uniform_int_distribution<int> _uid;
};

// 1. 상태 시간 누적
// 2. 액션 요청 상태 시간 누적
// 3. 감지 결과와 실행 결과를 보고 전술 상태 전이
// 4. 필요하면 쿨다운 리셋

// 기본 상태 전이 규칙
//
// 1. Idle
//  - 타겟 있으면 Reposition
// 
// 2. Reposition
//  - 타겟 잃으면 Idle
//  - 공격 가능하면 CommitAttack
// 
// 3. CommitAttack
//  - 요청이 실제 실행 중이면 Recover
//  - 요청이 거절되면 Reposition
//  - 너무 오래 걸리면 Reposition
// 
// 4. Recover
//  - 요청이 끝났고 최소 회복 시간이 지났으면 Reposition
//  - 타겟 잃으면 Idle