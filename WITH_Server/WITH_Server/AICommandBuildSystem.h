#pragma once

#include "System.h"

struct AIState;
struct AICommand;
struct AIBehavior;
struct ActionState;
struct AISenseState;
struct AICombatTuning;
struct AIActionRequestState;

class AICommandBuildSystem : public System {
public:
	AICommandBuildSystem(WorldRuntime& rt, int p = 0);
	virtual ~AICommandBuildSystem() = default;

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
	void BuildCommand(
		const ActionState& actionState,
		const AIState& aiState,
		const AISenseState& sense,
		const AIBehavior& behavior,
		const AIActionRequestState& req,
		const AICombatTuning& tuning,
		AICommand& outCmd
	);

	XMFLOAT3 MakeStrafeDir(bool strafeLeft, const XMFLOAT3& toTargetDir) const;

	int RandomPercent();


	std::mt19937 _rng;
	std::uniform_int_distribution<int> _uid;
};

// 1. Idle
//  - 아무 명령도 없음 or Hold
// 
// 2. Reposition
//  - sense.targetTooFar -> Run Chase
//  - sense.targetTooClose -> Walk Retreat
//  - sense.targetInPreferredRange -> Walk Strafe or Hold
// 
// 3. CommitAttack
//  - 아직 요청 상태가 Requested / Running이 아니면 Attack command
//  - 이미 요청 중이거나 실행 중이면 Hold
// 
// 4. Recover
//  - 기본적으로 Hold

