#pragma once

#include "IAISpecialActionPolicy.h"

class BossAISpecialActionPolicy : public IAISpecialActionPolicy {
public:
	virtual ~BossAISpecialActionPolicy() = default;

	virtual void TickRuntime(AIContext& ctx, const double dtSec) const override;
	virtual bool TryIssuePreFSMAction(AIContext& ctx) const override;

private:
	static bool TryFillDirectionToCurrentTarget(
		AIContext& ctx,
		float& outX,
		float& outZ) noexcept;
};

