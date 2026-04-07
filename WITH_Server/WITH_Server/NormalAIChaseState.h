#pragma once

#include "IAIState.h"

class NormalAIChaseState : public IAIState {
public:
	virtual ~NormalAIChaseState() = default;

	virtual AIStateType Type() const override { return AIStateType::Chase; }

	virtual void Enter(AIContext& ctx) const override;

	virtual void DecisionUpdate(AIContext& ctx, const double decisionDT) const override;
	virtual void FrameUpdate(AIContext& ctx, const double dT) const override;
};

