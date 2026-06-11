#pragma once

#include "AIBehaviorDef.h"
#include "IAIMovementPolicy.h"
#include "IAIState.h"

#include <DirectXMath.h>

class DataDrivenAIMovementPolicy final : public IAIMovementPolicy {
public:
	virtual ~DataDrivenAIMovementPolicy() = default;

	virtual void BuildChaseIntent(AIContext& ctx) const override;
	virtual void BuildCombatIntent(AIContext& ctx) const override;
	virtual void BuildSearchIntent(AIContext& ctx) const override;
	virtual void BuildReturnHomeIntent(AIContext& ctx) const override;

private:
	static const AIMovementProfileDef* SelectMovementProfile(
		const AIContext& ctx) noexcept;

	static bool TryComputeDirectionToTarget(
		AIContext& ctx,
		DirectX::XMFLOAT3& outDirection);

	static int ResolveStrafeSign(
		AIContext& ctx,
		const AIMovementProfileDef& profile);

	static void ApplyMovementBehavior(
		AIContext& ctx,
		const AIMovementProfileDef& profile,
		const DirectX::XMFLOAT3& toTarget,
		AIMovementBehavior behavior);
};
