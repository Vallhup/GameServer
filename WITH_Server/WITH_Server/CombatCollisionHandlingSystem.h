#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Event.h"
#include "Bufs.h"
#include "Stats.h"
#include "Action.h"

class CombatCollisionHandlingSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<CombatCollisionHandlingSystem>(),
		"CombatCollisionHandlingSystem",
		std::array{
			ReadImmediate(ComponentRes<FinalAttribute>()),
			ReadImmediate(ComponentRes<ActionState>()),
			ReadImmediate(ComponentRes<Transform>()),
			WriteImmediate(ComponentRes<AttackState>()),
			WriteImmediate(ComponentRes<ParryBuf>()),
			WriteImmediate(ComponentRes<Vital>()),
			WriteImmediate(EventRes<CombatCollisionEvent>()),
			WriteImmediate(EventRes<ActionRequestEvent>()),
		}
	);

public:
	CombatCollisionHandlingSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~CombatCollisionHandlingSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

private:
	bool ConsumeHitOnce(const CombatCollisionEvent& event);

	void HandleClash(const CombatCollisionEvent& event);
	void HandleStrike(const CombatCollisionEvent& event);

	void HandleParry(Entity attacker, Entity victim, uint32 attackId);
	void HandleGuard(Entity attacker, Entity victim, uint32 attackId);
	void HandleHit(Entity attacker, Entity victim, uint32 attackId);
};

