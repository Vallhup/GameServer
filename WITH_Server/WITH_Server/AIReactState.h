#pragma once

#include "IAIState.h"

class AIReactState : public IAIState {
public:
	virtual ~AIReactState() = default;

	virtual AIStateType Type() const override { return AIStateType::React; }

	virtual void Enter(AIContext& ctx) const override;

	virtual void DecisionUpdate(AIContext& ctx, const double decisionDT) const override;
	virtual void FrameUpdate(AIContext& ctx, const double dT) const override;
};

