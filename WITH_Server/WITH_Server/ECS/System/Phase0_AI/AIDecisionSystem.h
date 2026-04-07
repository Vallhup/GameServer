#pragma once

#include "System.h"

#include "../../../AIFSMRegistry.h"

class AIDecisionSystem final : public System {
	static const SystemMeta kMeta;
	static constexpr int kMaxDecisionStepsPerFrame{ 4 };

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMeta; }

private:
	void RunFSM(
		AIContext& ctx,
		const AIFSMBundle& bundle
	);


	void ApplyPendingTransition(
		AIContext& ctx,
		const AIFSMBundle& bundle
	);

	AIFSMRegistry _fsmRegistry;
};
