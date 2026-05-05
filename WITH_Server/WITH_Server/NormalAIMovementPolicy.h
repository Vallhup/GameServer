#pragma once

#include "IAIMovementPolicy.h"

class NormalAIMovementPolicy : public IAIMovementPolicy {
public:
	virtual ~NormalAIMovementPolicy() = default;

	virtual void BuildChaseIntent(AIContext& ctx) const override;
	virtual void BuildCombatIntent(AIContext& ctx) const override;
	virtual void BuildSearchIntent(AIContext& ctx) const override;
	virtual void BuildReturnHomeIntent(AIContext& ctx) const override;

protected:
	void BuildDestinationIntent(
		AIContext& ctx,
		const DirectX::XMFLOAT3& destination,
		bool wantsRun) const;

	bool TryGetCurrentTargetPosition(
		AIContext& ctx,
		DirectX::XMFLOAT3& outTargetPos) const;
};
