#pragma once

#include "System.h"

struct AIState;
struct ActionState;
struct AISenseState;
struct AICombatTuning;
struct AIActionRequestState;

class AIActionRequestSystem : public System {
public:
	AIActionRequestSystem(WorldRuntime& rt, int p = 0);
	virtual ~AIActionRequestSystem() = default;

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
	void ProcessAttackRequest(
		Entity entity,
		const ActionState& actionState,
		AIState& aiState,
		const AISenseState& sense,
		const AICombatTuning& tuning,
		AIActionRequestState& req
	);

	AttackType SelectAttack(
		Entity self,
		const AIState& aiState,
		const AISenseState& sense,
		const AICombatTuning& tuning
	);

	int RandomPercent();


	std::mt19937 _rng;
	std::uniform_int_distribution<int> _uid;
};

// 상태 전이 규칙
//
// 1. None
//  - 아무것도 안 함
// 
// 2. Requested
//  - 아직 requestedAttack == None이면 공격 타입 선택
//  - 액션 요청 이벤트 발행
//  - 이후 실제 ActionState.action == Attack이 되면 Running
//  - 일정 시간 내 안 되면 Rejected
// 
// 3. Running
//  - ActionState.action != Attack이 되면 Finished
// 
// 4. Finished
//  - AiTacticsystem이 회복 상태에서 consume하고 Clear()
// 
// 5. Rejected
//  - AITacticsystem이 재배치 상태로 되돌아가며 Clear()
