#pragma once

#include "IAICombatActionPolicy.h"

class NormalAICombatActionPolicy final : public IAICombatActionPolicy {
public:
	virtual ~NormalAICombatActionPolicy() = default;

	virtual CombatActionSelection SelectAction(const AIContext& ctx) const override;
};
