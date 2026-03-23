#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Movement.h"
#include "Stats.h"
#include "Animation.h"
#include "Action.h"

class LocomotionMoveSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<LocomotionMoveSystem>(),
		"LocomotionMoveSystem",
		std::array{
			ReadImmediate(ComponentRes<Velocity>()),
			ReadImmediate(ComponentRes<LocomotionState>()),
			ReadImmediate(ComponentRes<ActionState>()),
			ReadImmediate(ComponentRes<FinalAttribute>()),
			WriteImmediate(ComponentRes<LocomotionMoveDelta>()),
			WriteImmediate(ComponentRes<LocomotionAnimPhase>())
		}
	);

public:
	LocomotionMoveSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~LocomotionMoveSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

private:
	void ApplyNormalMovement(LocomotionMoveDelta* moveDelta,
		LocomotionAnimPhase* animPhase, const LocomotionState& loco, 
		const Velocity& vel, const FinalAttribute& attribute, const double dT);
};

