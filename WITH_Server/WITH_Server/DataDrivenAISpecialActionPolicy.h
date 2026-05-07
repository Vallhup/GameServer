#pragma once

#include "IAISpecialActionPolicy.h"

class DataDrivenAISpecialActionPolicy final : public IAISpecialActionPolicy {
public:
	virtual ~DataDrivenAISpecialActionPolicy() = default;

	void TickRuntime(AIContext& ctx, const double dtSec) const override;
	bool TryIssuePreFSMAction(AIContext& ctx) const override;
};
