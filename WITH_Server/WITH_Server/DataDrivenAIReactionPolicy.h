#pragma once

#include "AIBehaviorDef.h"
#include "IAIReactionPolicy.h"
#include "IAIState.h"

class DataDrivenAIReactionPolicy final : public IAIReactionPolicy {
public:
	virtual ~DataDrivenAIReactionPolicy() = default;

	virtual ReactionDecision Evaluate(
		const AIReactionEvent& topEvent,
		const AIContext& ctx) const override;

private:
	struct ReactionSelectionContext
	{
		const AIContext&					aiCtx;
		std::span<const AIReactionRuleDef>	rules;

		AIReactionRuleEvent	event{ AIReactionRuleEvent::OnHpThreshold };
		uint8_t				phase{ 1 };
		float				selfHpRatio{ 1.0f };
	};

	static ReactionSelectionContext BuildSelectionContext(
		const AIReactionEvent& topEvent,
		const AIContext& ctx) noexcept;

	static const AIReactionRuleDef* SelectRule(
		const ReactionSelectionContext& selection) noexcept;

	static ReactionDecision BuildDecision(
		const AIReactionRuleDef& rule) noexcept;
};
