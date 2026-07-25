#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

#include "../../../AIFSMRegistry.h"

class AIDecisionSystem final : public System {
	static const StaticSystemMetaStorage<15, 1, 2> kMetaStorage;
	static constexpr int kMaxDecisionStepsPerFrame{ 4 };

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }

private:
	void RunFSM(
		AIContext& ctx,
		const AIFSMBundle& bundle);

	bool ApplyPendingTransition(
		AIContext& ctx,
		const AIFSMBundle& bundle);

	static bool TryIssueReactionAbility(
		AIContext& aiCtx,
		const ReactionDecision& reactionDecision) noexcept;

	static void ApplyReactDurationOverride(
		AIContext& aiCtx,
		const ReactionDecision& reactionDecision) noexcept;

	AIFSMRegistry _fsmRegistry;
};
