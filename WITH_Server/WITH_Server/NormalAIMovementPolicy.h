#pragma once

#include "IAIMovementPolicy.h"

class NormalAIMovementPolicy : public IAIMovementPolicy {
public:
	virtual ~NormalAIMovementPolicy() = default;

	virtual void BuildChaseIntent(AIContext& ctx) override;
	virtual void BuildCombatIntent(AIContext& ctx) override;
	virtual void BuildSearchIntent(AIContext& ctx) override;
	virtual void BuildReturnHomeIntent(AIContext& ctx) override;

protected:
	void BuildDestinationIntent(
		AIContext& ctx,
		const DirectX::XMFLOAT3& destination,
		bool wantsRun) const;

	bool TryGetCurrentTargetPosition(
		AIContext& ctx,
		DirectX::XMFLOAT3& outTargetPos
	) const;
};
