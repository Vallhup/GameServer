#pragma once

#include "System.h"

class ActionMoveSystem : public System {
public:
	ActionMoveSystem(WorldRuntime& rt, int p = 0) : System(rt, p) {}
	virtual ~ActionMoveSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return { /*typeid(Transform),*/ typeid(Velocity), typeid(ActionState), typeid(AIState) };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return { typeid(ActionMoveDelta), typeid(ActionMoveTag) };
	}

private:
	bool CanMove(ActionType type);

	bool AdvanceSegmentByTime(ActionMoveTag* actionMove, const ActionState& actionState, const std::vector<ActionMoveSegment>& segs);
	bool OnEnterSegment(ActionMoveTag* actionMove, const ActionMoveSegment& seg, Entity self, Entity targetEnt);
	void ForceNextSegment(ActionMoveTag* actionMove, const std::vector<ActionMoveSegment>& segs);
	bool GetLockDirectionFromVelocity(ActionMoveTag* actionMove, const Velocity& vel, bool lockDir, XMVECTOR& outDir);

	bool HandleFixedDIstance(ActionMoveTag* actionMove, ActionMoveDelta* actionDelta, const ActionMoveSegment& seg,
		const ActionState& actionState, const Velocity& vel, const double dT);

	bool HandleDashToTarget(ActionMoveTag* actionMove, ActionMoveDelta* actionDelta, const ActionMoveSegment& seg,
		const ActionState& actionState, const Transform& tr, const double dT);

	void ApplyActionMovement(ActionMoveTag* actionMove, 
		ActionMoveDelta* actionDelta, const ActionState& actionState,
		const Transform& trans, const Velocity& vel, Entity self, Entity targetEnt, const double dT);
};