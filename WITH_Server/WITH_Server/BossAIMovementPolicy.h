#pragma once

#include "NormalAIMovementPolicy.h"

class BossAIMovementPolicy final : public NormalAIMovementPolicy {
public:
	virtual ~BossAIMovementPolicy() = default;

	virtual void BuildCombatIntent(AIContext& ctx) const override;
};
