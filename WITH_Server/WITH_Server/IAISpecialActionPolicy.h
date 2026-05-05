#pragma once

struct AIContext;

class IAISpecialActionPolicy {
public:
	virtual ~IAISpecialActionPolicy() = default;

	virtual void TickRuntime(AIContext& ctx, const double dtSec) const = 0;
	virtual bool TryIssuePreFSMAction(AIContext& ctx) const = 0;
};