#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "AI.h"
#include "Tags.h"

class AIPerceptionSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<AIPerceptionSystem>(),
		"AIPerceptionSystem",
		std::array<AccessSpec, 0>{  }
	);

public:
	AIPerceptionSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~AIPerceptionSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

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


	bool IsTargetEntityValid(Entity self, Entity other) const;


	bool IsWithinLeash(
		const Transform& selfTr,
		const Transform& otherTr,
		const AIPerceptionTuning& tuning
	) const;


	double ComputeScore(
		const PerceptionCandidate& candidate,
		const AIPerceptionTuning& selfTuning
	) const;


	PerceptionCandidate EvaluateCandidate(
		Entity self,
		const Transform& selfTr,
		const Transform& otherTr,
		const AIPerceptionTuning& tuning,
		const AIBlackboard& blackboard,
		Entity other
	) const;

	void BuildPerception(
		Entity self,
		const Transform& selfTr,
		const AIPerceptionTuning& tuning,
		AIBlackboard& blackboard,
		AIPerceptionCache& cache,
		const double dT
	);
};

