#pragma once

#include "System.h"
#include <DirectXMath.h>

using namespace DirectX;

struct AIPerceptionTuningComp;
struct WorldTransformComp;
struct AIBlackboardComp;
struct AIPerceptionComp;

class AIPerceptionSystem final : public System {
public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override;

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

	static double DistanceSqXZ(
		const XMFLOAT3& a,
		const XMFLOAT3& b) noexcept;

	static void DirectionXZ(
		const XMFLOAT3& from,
		const XMFLOAT3& to,
		double& outX,
		double& outZ) noexcept;

	double ComputeScore(
		const PerceptionCandidate& candidate,
		const AIPerceptionTuningComp& tuning) const noexcept;

	PerceptionCandidate EvaluateCandidate(
		Entity self,
		const WorldTransformComp& selfTr,
		const WorldTransformComp& otherTr,
		const AIPerceptionTuningComp& tuning,
		const AIBlackboardComp& blackboard,
		Entity other) const noexcept;

	void BuildPerception(
		Entity self,
		const WorldTransformComp& selfTr,
		const AIPerceptionTuningComp& tuning,
		AIBlackboardComp& blackboard,
		AIPerceptionComp& cache,
		const ECSView& ecs,
		double dT) const;

	static const SystemMeta kMeta;
};
