#pragma once

struct AIContext;

class IMovementPolicy {
public:
	virtual ~IMovementPolicy() = default;

	virtual void BuildChaseIntent(AIContext& ctx) = 0;
	virtual void BuildCombatIntent(AIContext& ctx) = 0;
	virtual void BuildSearchIntent(AIContext& ctx) = 0;
};