#pragma once

#include "IAIState.h"

class AIIdleState : public IAIState {
public:
	virtual ~AIIdleState() = default;

	virtual AIStateType Type() const override { return AIStateType::Idle; }

	virtual void Enter(AIContext& ctx) const override;

	virtual void DecisionUpdate(AIContext& ctx, const double decisionDT) const override;
	virtual void FrameUpdate(AIContext& ctx, const double dT) const override;
};

