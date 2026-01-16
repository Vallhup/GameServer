#include "pch.h"
#include "ECS.h"
#include "Framework.h"
#include "MovementSystem.h"
#include "ActionMoveSystem.h"

void MovementSystem::Execute(const float dT)
{
	auto& transforms = ecs.GetStorage<Transform>();
	auto& velocitys = ecs.GetStorage<Velocity>();
	auto& locos = ecs.GetStorage<LocomotionState>();
	auto& actionStates = ecs.GetStorage<ActionState>();
	auto& actionMoves = ecs.GetStorage<ActionMoveTag>();
	auto& framework = Framework::Get();

	for (const auto& [entity, transform] : transforms)
	{
		if (auto* vel = velocitys.GetComponent(entity))
		{
			if (auto* loco = locos.GetComponent(entity))
			{
				if (auto* action = actionStates.GetComponent(entity))
				{
					if (!CanMove(action->type)) continue;
				
					if (action->type != ActionType::None)
					{
						if(auto* actionMove = actionMoves.GetComponent(entity))
 							ApplyActionMovement(entity, *actionMove, transform, *vel, dT);
					}

					else
						ApplyNormalMovement(entity, *loco, transform, *vel, dT);
				}
			}
		}
	}
}

bool MovementSystem::CanMove(ActionType type)
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

void MovementSystem::ApplyActionMovement(Entity entity, ActionMoveTag& action, Transform& trans, const Velocity& vel, const float dT)
{
	if (!action.profile) return;

	action.elapsed += dT;

	const auto& segments = action.profile->segments;
	if (action.segmentIndex >= segments.size()) return;

	const auto& seg = segments[action.segmentIndex];

	const float segStart = seg.t0 * action.profile->duration;
	const float segEnd = seg.t1 * action.profile->duration;

	if (action.elapsed < segStart) return;

	const float segDuration = segEnd - segStart;
	if (segDuration <= 0.0f) return;

	const float speed = seg.distance / segDuration;

	const float move = speed * dT;
	const float remain = seg.distance - action.movedInSegment;
	const float actual = std::min(move, remain);

	action.movedInSegment += actual;

	XMVECTOR dir;
	if (seg.lockDir && action.dirLocked)
	{
		dir = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&action.dir));
	}
	else
	{
		dir = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&vel.dir));
		dir = XMVector3Normalize(dir);

		if (seg.lockDir)
		{
			XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&action.dir), dir);
			action.dirLocked = true;
		}
	}

	// 위치 적용
	XMVECTOR pos = XMLoadFloat3(&trans.position);
	pos = XMVectorAdd(pos, XMVectorScale(dir, actual));
	XMStoreFloat3(&trans.position, pos);

	// 세그먼트 종료
	if (action.movedInSegment >= seg.distance)
	{
		action.segmentIndex++;
		action.movedInSegment = 0.0f;

		if (action.segmentIndex < segments.size())
		{
			if (!segments[action.segmentIndex].lockDir)
				action.dirLocked = false;
		}
	}

	Framework::Get().outEventQueue.push(OutputEvent{
		entity, DirtyType::Moved });
}

void MovementSystem::ApplyNormalMovement(Entity entity, const LocomotionState& loco, Transform& trans, const Velocity& vel, const float dT)
{
	if (loco.isMoving)
	{
		// TEMP : 걷기 뛰기에 따라 속도 조정
		const float speed = 2.0f;

		XMVECTOR pos = XMLoadFloat3(&trans.position);
		XMVECTOR dir = XMLoadFloat3(&vel.dir);

		pos = XMVectorAdd(pos, XMVectorScale(dir, speed * dT));
		XMStoreFloat3(&trans.position, pos);

		float moveYaw = atan2f(
			-XMVectorGetX(dir),
			-XMVectorGetZ(dir)
		);

		XMVECTOR q = XMQuaternionRotationRollPitchYaw(0, moveYaw, 0);
		XMStoreFloat4(&trans.rotation, q);

		Framework::Get().outEventQueue.push(OutputEvent{
			entity, DirtyType::Moved });
	}
}
