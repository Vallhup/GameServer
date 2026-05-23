#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

struct AIPerceptionTuningDef;
struct AITargetingTuningDef;
struct WorldTransformComp;
struct AIBlackboardComp;
struct AIPerceptionComp;

class AIPerceptionSystem final : public System {
	static const StaticSystemMetaStorage<6> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }

private:
	struct PerceptionCandidate
	{
		Entity entity{ Entity::Null() };

		bool isCurrentTarget{ false };
		bool isLastAttacker{ false };

		double distSq{ 0.0 };

		bool inSightRange{ false };
		bool inAttackRange{ false };

		double forwardDot{ 0.0 };
		bool inFront{ false };

		bool visible{ false };

		double score{ 0.0 };
	};


	static void EnterReturnHome(
		AIBlackboardComp& blackboard,
		AIPerceptionComp& perception);

	double ComputeScore(
		const PerceptionCandidate& candidate,
		const AIPerceptionTuningDef& perception,
		const AITargetingTuningDef& targeting
	) const noexcept;


	PerceptionCandidate EvaluateCandidate(
		const WorldTransformComp& selfTr,
		const WorldTransformComp& otherTr,
		const AIPerceptionTuningDef& tuning,
		const AITargetingTuningDef& targeting,
		const AIBlackboardComp& blackboard,
		Entity other
	) const noexcept;


	void BuildPerception(
		Entity self,
		const WorldTransformComp& selfTr,
		const AIPerceptionTuningDef& tuning,
		const AITargetingTuningDef& targeting,
		AIBlackboardComp& blackboard,
		AIPerceptionComp& perception,
		SystemContext& sysCtx
	);
};
