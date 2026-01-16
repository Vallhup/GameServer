#include "pch.h"
#include "ActionMoveSystem.h"
#include "Framework.h"

void ActionMoveSystem::Execute(const float dT)
{
	auto& transforms = ecs.GetStorage<Transform>();
	auto& velocities = ecs.GetStorage<Velocity>();
	auto& actionStates = ecs.GetStorage<ActionState>();
	auto& actionMoves = ecs.GetStorage<ActionMoveTag>();

	for (const auto& [entity, transform] : transforms)
	{
		auto* vel = velocities.GetComponent(entity);
		auto* action = actionStates.GetComponent(entity);
		auto* actionMove = actionMoves.GetComponent(entity);

		if(!vel || !action || !actionMove) continue;
		if (action->type == ActionType::None) continue;
		if (!CanMove(action->type)) continue;

		ApplyActionMovement(entity, *actionMove, transform, *vel, dT);
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

void ActionMoveSystem::ApplyActionMovement(Entity entity, 
	ActionMoveTag& actionMove, Transform& trans, const Velocity& vel, 
	const float dT)
{
	if (!actionMove.profile) return;

	actionMove.elapsed += dT;

	const auto& segments = actionMove.profile->segments;
	if (actionMove.segmentIndex >= segments.size()) return;

	const auto& seg = segments[actionMove.segmentIndex];

	const float segStart = seg.t0 * actionMove.profile->duration;
	const float segEnd = seg.t1 * actionMove.profile->duration;

	if (actionMove.elapsed < segStart) return;

	const float segDuration = segEnd - segStart;
	if (segDuration <= 0.0f) return;

	const float speed = seg.distance / segDuration;

	const float move = speed * dT;
	const float remain = seg.distance - actionMove.movedInSegment;
	const float actual = std::min(move, remain);

	actionMove.movedInSegment += actual;

	XMVECTOR dir;
	if (seg.lockDir && actionMove.dirLocked)
	{
		dir = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&actionMove.dir));
	}
	else
	{
		dir = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&vel.dir));
		dir = XMVector3Normalize(dir);

		if (seg.lockDir)
		{
			XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&actionMove.dir), dir);
			actionMove.dirLocked = true;
		}
	}

	// 위치 적용
	XMVECTOR pos = XMLoadFloat3(&trans.position);
	pos = XMVectorAdd(pos, XMVectorScale(dir, actual));
	XMStoreFloat3(&trans.position, pos);

	// 세그먼트 종료
	if (actionMove.movedInSegment >= seg.distance)
	{
		actionMove.segmentIndex++;
		actionMove.movedInSegment = 0.0f;

		if (actionMove.segmentIndex < segments.size())
		{
			if (!segments[actionMove.segmentIndex].lockDir)
				actionMove.dirLocked = false;
		}
	}

	Framework::Get().outEventQueue.push(OutputEvent{
		entity, DirtyType::Moved });
}