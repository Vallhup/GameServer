#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Tags.h"
#include "Intent.h"
#include "Command.h"
#include "Movement.h"

#include "Event.h"

class EntityCommandConsumeSystem final : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<EntityCommandConsumeSystem>(),
		"EntityCommandConsumeSystem",
		std::array<AccessSpec, 0>{  }
	);

public:
	EntityCommandConsumeSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~EntityCommandConsumeSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

private:
	void ConsumeMove(
		Entity entity,
		const EntityCommandFrame& command,
		Velocity& velocity,
		LocomotionState& locomotion
	) const;


	void ConsumeLook(
		Entity entity,
		const EntityCommandFrame& command
	) const;


	void ConsumeGuard(
		Entity entity,
		const EntityCommandFrame& command,
		ActionIntent& actionIntent
	) const;


	void ConsumeAction(
		Entity entity,
		const EntityCommandFrame& command
	) const;
};
