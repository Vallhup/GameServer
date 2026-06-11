#pragma once

#include "IAIState.h"

class AIChaseState : public IAIState
{
public:
	virtual ~AIChaseState() = default;

	virtual AIStateType Type() const override { return AIStateType::Chase; }

	virtual void Enter(AIContext& ctx) const override;

	virtual void DecisionUpdate(AIContext& ctx, const double decisionDT) const override;
	virtual void FrameUpdate(AIContext& ctx, const double dT) const override;

private:
	static bool ShouldReturnHome(const AIContext& ctx) noexcept;
	static bool ShouldSearchForTarget(const AIContext& ctx) noexcept;
};

