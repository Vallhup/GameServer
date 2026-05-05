#pragma once

#include "IAIState.h"

class AISearchState final : public IAIState {
public:
	virtual ~AISearchState() = default;

	virtual AIStateType Type() const override { return AIStateType::Search; }

	virtual void Enter(AIContext& ctx) const override;

	virtual void DecisionUpdate(AIContext& ctx, const double decisionDT) const override;
	virtual void FrameUpdate(AIContext& ctx, const double dT) const override;
};
