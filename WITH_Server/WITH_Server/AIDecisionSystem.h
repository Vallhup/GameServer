#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "AIFSMRegistry.h"

class AIDecisionSystem final : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<AIDecisionSystem>(),
		"AIDecisionSystem",
		std::array<AccessSpec, 0>{  }
	);

	static constexpr int kMaxDecisionStepPerFrame{ 4 };

public:
	AIDecisionSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~AIDecisionSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

private:
	void RunFSM(
		AIContext& ctx, 
		const AIFSMBundle& bundle,
		const double dT
	);


	void ApplyPendingTransition(
		AIContext& ctx,
		const AIFSMBundle& bundle
	);


	AIFSMRegistry _fsmRegistry;
};

