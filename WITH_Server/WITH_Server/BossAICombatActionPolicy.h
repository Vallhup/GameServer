#pragma once

#include "IAICombatActionPolicy.h"

class BossAICombatActionPolicy final : public IAICombatActionPolicy {
public:
	virtual ~BossAICombatActionPolicy() = default;

	virtual CombatActionSelection SelectAction(const AIContext& ctx) const override;
};
