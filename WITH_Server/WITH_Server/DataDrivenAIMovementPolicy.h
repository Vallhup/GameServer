#pragma once

#include "IAIMovementPolicy.h"

class DataDrivenAIMovementPolicy final : public IAIMovementPolicy {
public:
	virtual ~DataDrivenAIMovementPolicy() = default;

	void BuildChaseIntent(AIContext& ctx) const override;
	void BuildCombatIntent(AIContext& ctx) const override;
	void BuildSearchIntent(AIContext& ctx) const override;
	void BuildReturnHomeIntent(AIContext& ctx) const override;
};
