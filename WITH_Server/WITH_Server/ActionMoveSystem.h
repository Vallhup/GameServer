#pragma once

#include "System.h"

#include "AI.h"
#include "Action.h"
#include "Movement.h"

class ActionMoveSystem : public System {
public:
	ActionMoveSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~ActionMoveSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { typeid(Transform), typeid(Velocity), typeid(ActionState), typeid(AIState) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(ActionMoveDelta), typeid(ActionMoveTag) };
	}

private:
	bool CanMove(ActionType action, EntityType entity, AttackType attack);

	bool AdvanceSegmentByTime(ActionMoveTag* actionMove, const ActionState& actionState, const std::vector<ActionMoveSegment>& segs);
	bool OnEnterSegment(ActionMoveTag* actionMove, const ActionMoveSegment& seg, const Transform& trans, Entity self, Entity targetEnt);
	void ForceNextSegment(ActionMoveTag* actionMove, const std::vector<ActionMoveSegment>& segs);
	bool GetLockDirection(ActionMoveTag* actionMove, const Transform& trans, const Velocity& vel, bool lockDir, XMVECTOR& outDir);

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
