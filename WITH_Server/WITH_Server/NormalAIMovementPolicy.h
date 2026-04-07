#pragma once

#include "IAIMovementPolicy.h"

class NormalAIMovementPolicy : public IAIMovementPolicy {
public:
	virtual ~NormalAIMovementPolicy() = default;

	virtual void BuildChaseIntent(AIContext& ctx) override;
	virtual void BuildCombatIntent(AIContext& ctx) override;
	virtual void BuildSearchIntent(AIContext& ctx) override;

private:
	bool TryGetCurrentTargetPosition(
		AIContext& ctx,
		DirectX::XMFLOAT3& outTargetPos
	) const;
};
