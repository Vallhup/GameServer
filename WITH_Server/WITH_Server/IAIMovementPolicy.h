#pragma once

struct AIContext;

class IAIMovementPolicy {
public:
	virtual ~IAIMovementPolicy() = default;

	virtual void BuildChaseIntent(AIContext& ctx) const = 0;
	virtual void BuildCombatIntent(AIContext& ctx) const = 0;
	virtual void BuildSearchIntent(AIContext& ctx) const = 0;
	virtual void BuildReturnHomeIntent(AIContext& ctx) const = 0;
};
