#pragma once

#include "IAICombatActionPolicy.h"

class WeightedCombatActionPolicy final : public IAICombatActionPolicy {
public:
	CombatActionSelection SelectAction(const AIContext& ctx) const override;
};
