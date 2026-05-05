#pragma once

#include "IAIReactionPolicy.h"

class BossAIReactionPolicy final : public IAIReactionPolicy {
public:
	virtual ~BossAIReactionPolicy() = default;

	virtual ReactionDecision Evaluate(
		const AIReactionEvent& topEvent,
		const AIContext& ctx) const override;
};
