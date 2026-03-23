#pragma once

#include "IAIState.h"

class NormalAICombatState final : public IAIState {
public:
	virtual ~NormalAICombatState() = default;

	virtual AIStateType Type() const override { return AIStateType::Combat; }

	virtual void Enter(AIContext& ctx) const override;

	virtual void DecisionUpdate(AIContext& ctx, const double decisionDT) const override;
	virtual void FrameUpdate(AIContext& ctx, const double dT) const override;
};

