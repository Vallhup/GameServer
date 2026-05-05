#pragma once

#include "IAIState.h"

class AIReturnHomeState final : public IAIState {
public:
	virtual ~AIReturnHomeState() = default;

	virtual AIStateType Type() const override { return AIStateType::ReturnHome; }

	virtual void Enter(AIContext& ctx) const override;

	virtual void DecisionUpdate(AIContext& ctx, const double decisionDT) const override;
	virtual void FrameUpdate(AIContext& ctx, const double dT) const override;
};
