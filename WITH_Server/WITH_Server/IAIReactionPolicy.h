#pragma once

#include "ECS/GameplayRuntimeComponents.h"
#include "GameplayContentIds.h"

struct AIContext;

enum class ReactionTacticalOutcome : uint8_t
{
	Ignore,
	EnterReact,
	ForceRetarget,
	IssueAbility,
	EnterReactAndIssueAbility,
};

struct ReactionDecision
{
	ReactionTacticalOutcome outcome{ ReactionTacticalOutcome::Ignore };
	bool retargetAttacker{ false };
	AbilityId reactAbilityId{ InvalidAbilityId };
	bool hasReactDurationOverride{ false };
	float reactDurationSec{ 0.0f };
};

class IAIReactionPolicy {
public:
	virtual ~IAIReactionPolicy() = default;

	virtual ReactionDecision Evaluate(
		const AIReactionEvent& topEvent,
		const AIContext& ctx) const = 0;
};
