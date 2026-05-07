#pragma once

#include "IAICombatActionPolicy.h"

class DataDrivenAICombatActionPolicy final : public IAICombatActionPolicy {
public:
	virtual ~DataDrivenAICombatActionPolicy() = default;

	CombatActionSelection SelectAction(const AIContext& ctx) const override;
};
