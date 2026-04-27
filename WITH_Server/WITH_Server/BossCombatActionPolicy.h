#pragma once

#include "IAICombatActionPolicy.h"

class BossCombatActionPolicy final : public IAICombatActionPolicy
{
public:
	CombatActionSelection SelectAction(const AIContext& ctx) const override;
};
