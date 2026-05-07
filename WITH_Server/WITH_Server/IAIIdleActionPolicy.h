#pragma once

#include "IAIState.h"

class IAIIdleActionPolicy {
public:
	virtual ~IAIIdleActionPolicy() = default;

	virtual void TryIssueIdleAction(AIContext& ctx) const = 0;
};