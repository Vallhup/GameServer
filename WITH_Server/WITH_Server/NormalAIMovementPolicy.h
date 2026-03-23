#pragma once

#include "IMovementPolicy.h"

class NormalAIMovementPolicy final : public IMovementPolicy {
public:
	virtual ~NormalAIMovementPolicy() = default;

	virtual void BuildChaseIntent(AIContext& ctx) override;
	virtual void BuildCombatIntent(AIContext& ctx) override;
	virtual void BuildSearchIntent(AIContext& ctx) override;

private:
	bool TryGetCurrentTargetPosition(
		AIContext& ctx, 
		XMFLOAT3& outTargetPos
	) const;
};

