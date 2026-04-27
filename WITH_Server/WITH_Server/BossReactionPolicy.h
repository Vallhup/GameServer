#pragma once

#include "IAIReactionPolicy.h"

class BossReactionPolicy final : public IAIReactionPolicy
{
public:
	virtual ~BossReactionPolicy() = default;

	virtual ReactionDecision Evaluate(
		const AIReactionEvent& topEvent,
		const AIContext& ctx) const override;
};
