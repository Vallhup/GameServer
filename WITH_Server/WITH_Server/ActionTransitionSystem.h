#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "Event.h"
#include "Action.h"
#include "Intent.h"

class ActionTransitionSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<ActionTransitionSystem>(),
		"ActionTransitionSystem",
		std::array{
			ReadImmediate(ComponentRes<Transform>()),
			ReadImmediate(ComponentRes<ActionIntent>()),
			WriteImmediate(ComponentRes<ActionMoveTag>()),
			WriteImmediate(ComponentRes<ActionState>()),
			WriteImmediate(EventRes<ActionRequestEvent>())
		}
	);

public:
	ActionTransitionSystem(WorldRuntime& rt);
	virtual ~ActionTransitionSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

private:
	std::pair<ActionType, AttackType> ResolveNextAction(EntityType type, const ActionState& current, ActionRequestEvent request, bool guardHeld, bool isForced);
	void ApplyTransition(Entity entity, EntityType type, ActionState* state, ActionType next, AttackType nextAttack);
	std::span<ActionRequestEvent> DedupActionRequest(std::span<ActionRequestEvent> events);
	

	void LoadTransitionRules();
	void SetRule(ActionType cur, ActionType req, ActionType next);
	ActionType GetRule(ActionType cur, ActionType req) const;

	static constexpr ActionType Invalid = static_cast<ActionType>(255);
	std::array<std::array<ActionType, actionCnt>, actionCnt> _transitionRules;
};

