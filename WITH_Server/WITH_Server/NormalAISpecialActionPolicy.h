#pragma once

#include "IAISpecialActionPolicy.h"

class NormalAISpecialActionPolicy : public IAISpecialActionPolicy {
public:
	virtual ~NormalAISpecialActionPolicy() = default;

	virtual void TickRuntime(AIContext& ctx, const double dtSec) const override;
	virtual bool TryIssuePreFSMAction(AIContext& ctx) const override;
};

