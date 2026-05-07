#pragma once

#include "AIBehaviorDef.h"
#include "IAIIdleActionPolicy.h"
#include "IAIState.h"

class DataDrivenAIIdleActionPolicy final : public IAIIdleActionPolicy {
public:
	virtual ~DataDrivenAIIdleActionPolicy() = default;

	virtual void TryIssueIdleAction(AIContext& ctx) const override;

private:
	struct IdleActionCandidate
	{
		size_t   actionIndex{ 0 };
		uint32_t weight{ 0 };
	};

	struct IdleActionSelectionContext
	{
		AIContext&						aiCtx;
		AIActionRuntimeComp&			runtime;
		std::span<const AIActionDef>	actions;

		size_t  	runtimeOffset{ 0 };
		uint32_t	sequence{ 0 };
		uint8_t		phase{ 1 };
		float		selfHpRatio{ 1.0f };
		AbilityId	lastUsedAbilityId{ InvalidAbilityId };
	};

	static bool CanStartIdleAction(AIContext& ctx) noexcept;

	static IdleActionSelectionContext BuildSelectionContext(AIContext& ctx) noexcept;

	static std::vector<IdleActionCandidate> CollectCandidates(
		const IdleActionSelectionContext& selection);

	static const AIActionDef* PickIdleAction(
		const IdleActionSelectionContext& selection,
		std::span<const IdleActionCandidate> candidates,
		size_t& outActionIndex) noexcept;

	static void IssueIdleAction(
		const IdleActionSelectionContext& selection,
		const AIActionDef& action) noexcept;

	static void ApplyRuntimeSelection(
		const IdleActionSelectionContext& selection,
		const AIActionDef& action,
		size_t runtimeIndex);
};

