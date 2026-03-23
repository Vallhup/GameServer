#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "AI.h"
#include "Action.h"

class AIExecutionSystem final : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<AIExecutionSystem>(),
		"AIExecutionSystem",
		std::array<AccessSpec, 0>{  }
	);

public:
	AIExecutionSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~AIExecutionSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

private:
	void ExecuteMove(
		const ActionState& actionState,
		const AIIntent& aiIntent,
		const AIExecutionState& execState,
		EntityCommandFrame& entityCommand
	);


	void ExecuteLook(
		const AIIntent& aiIntent,
		EntityCommandFrame& entityCommand
	);

	
	void ExecuteAction(
		const ActionState& actionState,
		const AIIntent& aiIntent,
		AIExecutionState& execState,
		EntityCommandFrame& entityCommand
	);


	static bool CanWriteMove(const ActionState& actionState);
	static bool CanWriteAction(const ActionState& actionState);
};

