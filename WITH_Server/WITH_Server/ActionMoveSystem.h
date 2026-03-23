#pragma once

#include "System.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

#include "AI.h"
#include "Action.h"
#include "Movement.h"

class ActionMoveSystem : public System {
	static inline auto kMeta = MakeMetaStorage(
		SysTag<ActionMoveSystem>(),
		"ActionMoveSystem",
		std::array{
			ReadImmediate(ComponentRes<Transform>()),
			ReadImmediate(ComponentRes<Velocity>()),
			ReadImmediate(ComponentRes<ActionState>()),
			ReadImmediate(ComponentRes<AIState>()),
			WriteImmediate(ComponentRes<ActionMoveDelta>()),
			WriteImmediate(ComponentRes<ActionMoveTag>())
		}
	);

public:
	ActionMoveSystem(WorldRuntime& rt) : System(rt) {}
	virtual ~ActionMoveSystem() = default;

	virtual void Execute(const double dT) override;
	virtual const SystemMeta& Meta() const override { return kMeta.meta; }

private:
	bool CanMove(ActionType action, EntityType entity, AttackType attack);

	bool AdvanceSegmentByTime(ActionMoveTag* actionMove, const ActionState& actionState, const std::vector<ActionMoveSegment>& segs, EntityType type);
	bool OnEnterSegment(ActionMoveTag* actionMove, const ActionMoveSegment& seg, const Transform& trans, Entity self, Entity targetEnt, EntityType type);
	void ForceNextSegment(ActionMoveTag* actionMove, EntityType type);
	bool GetLockDirection(ActionMoveTag* actionMove, ActionMoveDelta* actionDelta, const Transform& trans, const Velocity& vel, bool lockDir, XMVECTOR& outDir);
	bool GetMoveBasisDirection(const ActionMoveDelta* actionDelta, const Transform& trans, const Velocity& vel, bool preferVelocity, XMVECTOR& outDir);

	bool ComputeYaw_FaceTarget(Entity self, Entity target, const Transform& trans, float& outYaw) const;

	bool HandleFixedDistance(ActionMoveTag* actionMove, ActionMoveDelta* actionDelta, const ActionMoveSegment& seg,
		const ActionState& actionState, const Transform& tr, const Velocity& vel, EntityType type, Entity target, const double dT);

	bool HandleDashToTarget(ActionMoveTag* actionMove, ActionMoveDelta* actionDelta, const ActionMoveSegment& seg,
		const ActionState& actionState, const Transform& tr, const double dT);

	bool HandleFixedDeltaY(ActionMoveTag* actionMove, ActionMoveDelta* actionDelta, const ActionMoveSegment& seg,
		const ActionState& actionState, const double dT);

	void ApplyActionMovement(ActionMoveTag* actionMove, 
		ActionMoveDelta* actionDelta, const ActionState& actionState,
		const Transform& trans, const Velocity& vel, EntityType type, Entity self, Entity targetEnt, const double dT);
};
