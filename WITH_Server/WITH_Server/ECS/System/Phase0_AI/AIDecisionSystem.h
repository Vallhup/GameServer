#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

#include "../../../AIFSMRegistry.h"

class AIDecisionSystem final : public System {
	static const StaticSystemMetaStorage<10, 1, 1> kMetaStorage;
	static constexpr int kMaxDecisionStepsPerFrame{ 4 };

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }

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
