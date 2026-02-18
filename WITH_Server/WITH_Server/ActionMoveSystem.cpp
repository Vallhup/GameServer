#include "pch.h"
#include "ActionMoveSystem.h"
#include "Framework.h"
#include "Math.h"

void ActionMoveSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	auto& velocities = ecs.GetStorage<Velocity>();
	auto& actionStates = ecs.GetStorage<ActionState>();
	auto& actionMoves = ecs.GetStorage<ActionMoveTag>();
	auto& actionDeltas = ecs.GetStorage<ActionMoveDelta>();

	for (const auto& [entity, actionState] : actionStates)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		auto* vel = velocities.GetComponent(entity);
		auto* actionMove = actionMoves.GetComponent(entity);
		auto* actionDelta = actionDeltas.GetComponent(entity);

		if(!vel || !actionMove || !actionDelta) continue;
		if (actionState.action == ActionType::None) continue;
		if (!CanMove(actionState.action)) continue;

		ApplyActionMovement(actionMove, actionDelta, actionState, 
			*vel, dT);
	}
}

bool ActionMoveSystem::CanMove(ActionType type)
{
	switch (type) {
	case ActionType::None:
	case ActionType::Attack:
	case ActionType::Dodge:
		return true;

	case ActionType::Guard:
	case ActionType::Parry:
	case ActionType::Hit:
	case ActionType::Stun:
	case ActionType::Dead:
		return false;

	default:
		return false;
	}
}

void ActionMoveSystem::ApplyActionMovement(
	ActionMoveTag* actionMove, ActionMoveDelta* actionDelta,
	const ActionState& actionState, const Velocity& vel, const double dT)
{
	if (!actionMove->profile) return;

	const auto& segments = actionMove->profile->segments;
	if (actionMove->segmentIndex >= segments.size()) return;

	const auto& seg = segments[actionMove->segmentIndex];

	const double segStart = seg.t0 * actionState.duration;
	const double segEnd = seg.t1 * actionState.duration;

	if (actionState.elapsed < segStart) return;

	const double segDuration = segEnd - segStart;
	if (segDuration <= 0.0f) return;

	const double speed = seg.distance / segDuration;

	const double move = speed * dT;
	const double remain = seg.distance - actionMove->movedInSegment;
	const double actual = std::min(move, remain);

	XMVECTOR dir;
	XMVECTOR out;

	if (seg.lockDir && actionMove->dirLocked)
	{
		out = XMLoadFloat3(&actionMove->dir);
	}

	else
	{
		dir = XMLoadFloat3(&vel.dir);
		if (!TransformHelper::SafeNormalize3(dir, out)) return;

		if (seg.lockDir)
		{
			XMStoreFloat3(&actionMove->dir, out);
			actionMove->dirLocked = true;
		}
	}

	if (actual > 1e-6f)
	{
		XMFLOAT3 deltaMove;
		XMStoreFloat3(&deltaMove, XMVectorScale(out, actual));

  		actionDelta->hasMove = true;
		actionDelta->deltaPos = deltaMove;

		const double yaw = atan2f(-deltaMove.x, -deltaMove.z);
		actionDelta->hasYaw = true;
		actionDelta->yaw = yaw;
	}

	// 세그먼트 종료
	actionMove->movedInSegment += actual;
	if (actionMove->movedInSegment >= seg.distance)
	{
		actionMove->segmentIndex++;
		actionMove->movedInSegment = 0.0f;

		if (actionMove->segmentIndex < segments.size())
		{
			if (!segments[actionMove->segmentIndex].lockDir)
				actionMove->dirLocked = false;
		}
	}
}