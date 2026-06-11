#pragma once

#include "../../../AITargetSelectionTypes.h"
#include "System.h"
#include "SystemMetaStorage.h"

struct AIPerceptionTuningDef;
struct AITargetingTuningDef;
struct WorldTransformComp;
struct AIBlackboardComp;
struct AIPerceptionComp;

class AIPerceptionSystem final : public System {
	static const StaticSystemMetaStorage<7> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }

private:
	static void EnterReturnHome(
		AIBlackboardComp& blackboard,
		AIPerceptionComp& perception);

	double ComputeScore(
		const AITargetCandidate& candidate,
		const AIPerceptionTuningDef& perception,
		const AITargetingTuningDef& targeting
	) const noexcept;

	AITargetCandidate EvaluateCandidate(
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
