#pragma once

#include "IAIReactionPolicy.h"

class DataDrivenAIReactionPolicy final : public IAIReactionPolicy {
public:
	virtual ~DataDrivenAIReactionPolicy() = default;

	ReactionDecision Evaluate(
		const AIReactionEvent& topEvent,
		const AIContext& ctx) const override;
};
