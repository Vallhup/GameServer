#include "pch.h"
#include "ActionMoveSystem.h"
#include "Framework.h"
#include "Math.h"
#include "RepComponent.h"

void ActionMoveSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();

	auto& transforms = ecs.GetStorage<Transform>();
	auto& velocities = ecs.GetStorage<Velocity>();
	auto& actionStates = ecs.GetStorage<ActionState>();
	auto& actionMoves = ecs.GetStorage<ActionMoveTag>();
	auto& actionDeltas = ecs.GetStorage<ActionMoveDelta>();
	auto& aiStates = ecs.GetStorage<AIState>();
	auto& spawnComps = ecs.GetStorage<SpawnTypeComp>();

	for (const auto& [entity, actionState] : actionStates)
	{
		if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity)) continue;

		auto* trans = transforms.GetComponent(entity);
		auto* vel = velocities.GetComponent(entity);
		auto* actionMove = actionMoves.GetComponent(entity);
		auto* actionDelta = actionDeltas.GetComponent(entity);
		auto* aiState = aiStates.GetComponent(entity);
		const auto* spawnComp = spawnComps.GetComponent(entity);

		if (!trans || !vel || !actionMove || !actionDelta || !spawnComp) continue;
		if (spawnComp->type == EntityType::Final_Boss && !aiState) continue;
		if (actionState.action == ActionType::None) continue;
		if (!CanMove(actionState.action, spawnComp->type, actionState.attack)) continue;
		
		Entity target = Entity::Null();
		if (aiState) target = aiState->target;

		actionDelta->hasMove = false;
		actionDelta->hasYaw = false;
		actionDelta->deltaPos = XMFLOAT3{ 0, 0, 0 };
		actionDelta->yaw = 0.0f;

		ApplyActionMovement(actionMove, actionDelta, actionState, *trans, *vel, spawnComp->type, entity, target, dT);
	}
}

bool ActionMoveSystem::CanMove(ActionType action, EntityType entity, AttackType attack)
{
	const auto& policy = ActionManager::Get().GetPolicy(action, entity, attack);
	return policy.isMoveAction;
}

bool ActionMoveSystem::AdvanceSegmentByTime(ActionMoveTag* actionMove, const ActionState& actionState, const std::vector<ActionMoveSegment>& segs)
{
	bool advanced{ false };
	while (actionMove->segmentIndex < segs.size())
	{
		const auto& seg = segs[actionMove->segmentIndex];
		const double segEnd = seg.t1 * actionState.duration;

		if (actionState.elapsed < segEnd)
			break;

		advanced = true;

		actionMove->segmentIndex++;
		actionMove->movedInSegment = 0.0f;
		actionMove->lastSegmentIndex = -1;
		actionMove->dashTraveled = 0.0;
		actionMove->dashHasTarget = false;
		actionMove->dirLocked = false;
		actionMove->yawLocked = false;
	}
	
	return advanced;
}

bool ActionMoveSystem::OnEnterSegment(ActionMoveTag* actionMove, const ActionMoveSegment& seg, const Transform& trans, Entity self, Entity targetEnt)
{
	if (actionMove->lastSegmentIndex == actionMove->segmentIndex)
		return false;

	actionMove->lastSegmentIndex = actionMove->segmentIndex;
	actionMove->movedInSegment = 0.0;
	actionMove->dashTraveled = 0.0;
	actionMove->dashHasTarget = false;

	if (!seg.moveParams.lockDir)
		actionMove->dirLocked = false;

	if (seg.moveMode == MoveMode::DashToTarget)
	{
		if (!targetEnt.IsNull())
		{
			if (const auto* targetTr = _runtime.GetECS().GetStorage<Transform>().GetComponent(targetEnt))
			{
				actionMove->dashTargetPos = targetTr->position;
				actionMove->dashHasTarget = true;
			}
		}
	}

	actionMove->yawLocked = false;
	if (seg.yawMode == YawMode::FaceTarget)
	{
		float desiredYaw;
		if (ComputeYaw_FaceTarget(self, targetEnt, trans, desiredYaw))
		{
			actionMove->yaw = desiredYaw;
			actionMove->yawLocked = true;
		}
	}

	return true;
}

void ActionMoveSystem::ForceNextSegment(ActionMoveTag* actionMove, const std::vector<ActionMoveSegment>& segs)
{
	actionMove->segmentIndex++;
	actionMove->lastSegmentIndex = -1;
	actionMove->movedInSegment = 0.0;
	actionMove->dashTraveled = 0.0;
	actionMove->dashHasTarget = false;
	actionMove->dirLocked = false;
	actionMove->yawLocked = false;
}

bool ActionMoveSystem::GetLockDirection(ActionMoveTag* actionMove, const Transform& trans, const Velocity& vel, bool lockDir, XMVECTOR& outDir)
{
	if (lockDir && actionMove->dirLocked)
	{
		outDir = XMLoadFloat3(&actionMove->dir);
		return true;
	}

	XMVECTOR dir = XMLoadFloat3(&vel.dir);
	if (!TransformHelper::SafeNormalize3(dir, outDir))
	{
		XMVECTOR forward = TransformHelper::Forward(trans);
		if (!TransformHelper::SafeNormalize3(forward, outDir))
			return false;
	}

	if (lockDir)
	{
		XMStoreFloat3(&actionMove->dir, outDir);
		actionMove->dirLocked = true;
	}

	return true;
}

bool ActionMoveSystem::ComputeYaw_FaceTarget(Entity self, Entity target, const Transform& trans, float& outYaw) const
{
	if (target.IsNull()) return false;

	const auto* targetTr = _runtime.GetECS().GetStorage<Transform>().GetComponent(target);
	if (!targetTr) return false;

	XMVECTOR selfPos = XMLoadFloat3(&trans.position);
	XMVECTOR targetPos = XMLoadFloat3(&targetTr->position);

	XMVECTOR toTarget = XMVectorSubtract(targetPos, selfPos);
	toTarget = XMVectorSetY(toTarget, 0.0f);

	const float lenSq = XMVectorGetX(XMVector3LengthSq(toTarget));
	if (lenSq < 1e-6f) return false;

	XMVECTOR norm;
	if (!TransformHelper::SafeNormalize3(toTarget, norm))
		return false;

	XMFLOAT3 dir;
	XMStoreFloat3(&dir, norm);

	outYaw = std::atan2(-dir.x, -dir.z);
	return true;
}

bool ActionMoveSystem::HandleFixedDistance(ActionMoveTag* actionMove, ActionMoveDelta* actionDelta, const ActionMoveSegment& seg, const ActionState& actionState, 
	const Transform& tr, const Velocity& vel, EntityType type, Entity target, const double dT)
{
	const double segStart = seg.t0 * actionState.duration;
	const double segEnd = seg.t1 * actionState.duration;
	const double segDuration = segEnd - segStart;
	if (segDuration <= 0.0) return true;

	const double dist = static_cast<double>(seg.moveParams.distance);
	if (dist <= 1e-9) return true;

	const double speed = dist / segDuration;
	const double move = speed * dT;
	if (move <= 0.0) return true;

	const double remain = dist - actionMove->movedInSegment;
	const double actual = std::min(move, remain);

	int8 dirMul = seg.moveParams.dirMul;
	if (dirMul != 1 && dirMul != -1) dirMul = 1;

	const float signedActual = actual * float(dirMul);

	XMVECTOR dir;
	bool gotDir{ false };

	const bool isMonster = (type != EntityType::Knight);
	if (isMonster && !target.IsNull())
	{
		if (const auto* targetTr = _runtime.GetECS().GetStorage<Transform>().GetComponent(target))
		{
			XMVECTOR selfPos = XMLoadFloat3(&tr.position);
			XMVECTOR targetPos = XMLoadFloat3(&targetTr->position);
			XMVECTOR toTarget = XMVectorSubtract(targetPos, selfPos);
			toTarget = XMVectorSetY(toTarget, 0.0f);

			XMVECTOR norm;
			if (TransformHelper::SafeNormalize3(toTarget, norm))
			{
				dir = norm;
				gotDir = true;

				if (seg.moveParams.lockDir)
				{
					if (actionMove->dirLocked)
						dir = XMLoadFloat3(&actionMove->dir);

					else
					{
						XMStoreFloat3(&actionMove->dir, dir);
						actionMove->dirLocked = true;
					}
				}
			}
		}
	}

	if(!gotDir)
	{
		if (!GetLockDirection(actionMove, tr, vel, seg.moveParams.lockDir, dir))
			return false;
	}

	if (actual > 1e-6)
	{
		XMFLOAT3 deltaMove;
		XMStoreFloat3(&deltaMove, XMVectorScale(dir, signedActual));

		actionDelta->hasMove = true;
		actionDelta->deltaPos = deltaMove;
		actionMove->movedInSegment += actual;
	}

	return (actionMove->movedInSegment >= dist);
}

bool ActionMoveSystem::HandleDashToTarget(ActionMoveTag* actionMove, ActionMoveDelta* actionDelta, const ActionMoveSegment& seg, const ActionState& actionState, const Transform& tr, const double dT)
{
	if (!actionMove->dashHasTarget)
		return true;

	XMVECTOR pos = XMLoadFloat3(&tr.position);
	XMVECTOR targetPos = XMLoadFloat3(&actionMove->dashTargetPos);
	XMVECTOR toTarget = XMVectorSubtract(targetPos, pos);

	const float dist = XMVectorGetX(XMVector3Length(toTarget));
	const float stopRange = seg.moveParams.stopRange;

	if (dist <= stopRange + 1e-4f)
		return true;

	XMVECTOR dir;
	if (!TransformHelper::SafeNormalize3(toTarget, dir))
		return true;

	if (seg.moveParams.lockDir)
	{
		if (actionMove->dirLocked)
			dir = XMLoadFloat3(&actionMove->dir);

		else
		{
			XMStoreFloat3(&actionMove->dir, dir);
			actionMove->dirLocked = true;
		}
	}

	const double maxSpeed = static_cast<double>(seg.moveParams.maxSpeed);
	if (maxSpeed <= 1e-9) return true;

	const double move = maxSpeed * dT;

	const double maxTravel = static_cast<double>(seg.moveParams.maxTravel);
	const double travelRemain = maxTravel - actionMove->dashTraveled;
	if (travelRemain <= 1e-9)
		return true;

	const double distRemain = 
		std::max(0.0, static_cast<double>(dist) - static_cast<double>(stopRange));

	const double actual = std::min({ move, travelRemain, distRemain });
	if (actual > 1e-6)
	{
		XMFLOAT3 deltaMove;
		XMStoreFloat3(&deltaMove, XMVectorScale(dir, actual));

		actionDelta->hasMove = true;
		actionDelta->deltaPos = deltaMove;
		actionMove->dashTraveled += actual;
	}

	if (actionMove->dashTraveled >= maxTravel)
		return true;

	if (distRemain <= actual)
		return true;

	return false;
}

void ActionMoveSystem::ApplyActionMovement(ActionMoveTag* actionMove, ActionMoveDelta* actionDelta, const ActionState& actionState, 
	const Transform& trans, const Velocity& vel, EntityType type, Entity self, Entity targetEnt, const double dT)
{
	if (!actionMove->profile) return;

	const auto& segments = actionMove->profile->segments;
	if (actionMove->segmentIndex >= segments.size()) 
		return;

	AdvanceSegmentByTime(actionMove, actionState, segments);
	if (actionMove->segmentIndex >= segments.size()) 
		return;

	const auto& seg = segments[actionMove->segmentIndex];

	const double segStart = seg.t0 * actionState.duration;
	if (actionState.elapsed < segStart) return;

	OnEnterSegment(actionMove, seg, trans, self, targetEnt);

	// 1. Yaw 먼저 계산
	bool yawAligned{ false };
	if (seg.yawMode == YawMode::FaceTarget)
	{
		float desiredYaw{ 0.0f };
		if (actionMove->yawLocked)
			desiredYaw = actionMove->yaw;

		else
		{
			if (!ComputeYaw_FaceTarget(self, targetEnt, trans, desiredYaw))
				goto AFTER_YAW;
			
			actionMove->yaw = desiredYaw;
			actionMove->yawLocked = true;		
		}
		const float curYaw = TransformHelper::WrapPi(TransformHelper::QuaternionToYaw(trans.rotation));
		const float yawDelta = TransformHelper::AngleDelta(curYaw, desiredYaw);
		if (std::fabs(yawDelta) <= seg.yawParams.yawEpsRad)
		{
			actionDelta->hasYaw = true;
			actionDelta->yaw = desiredYaw;
			yawAligned = true;
		}
		
		else
		{
			const float maxStep = seg.yawParams.turnSpeedRad * dT;
			const float step = std::clamp(yawDelta, -maxStep, maxStep);

			actionDelta->hasYaw = true;
			actionDelta->yaw = TransformHelper::WrapPi(curYaw + step);
		}
	}

AFTER_YAW:;

	// 회전-only segment면, 회전 정렬되면 바로 skip
	if (seg.moveMode == MoveMode::None && seg.yawMode != YawMode::None && yawAligned)
	{
		ForceNextSegment(actionMove, segments);
		return;
	}

	// 2. 이후 Move 처리
	bool finished{ false };
	switch (seg.moveMode) {
	case MoveMode::FixedDistance:
	{
		finished = HandleFixedDistance(actionMove, actionDelta, seg, actionState, trans, vel, type, targetEnt, dT);
		break;
	}
	case MoveMode::DashToTarget:
	{
		finished = HandleDashToTarget(actionMove, actionDelta, seg, actionState, trans, dT);
		break;
	}
	default:
	{
		finished = false;
		break;
	}
	}

	// 3. YawMode::FaceMoveDir이면 이동 후 deltaPos로 yaw 계산
	if (seg.yawMode == YawMode::FaceMoveDir && actionDelta->hasMove)
	{
		const float dx = actionDelta->deltaPos.x;
		const float dz = actionDelta->deltaPos.z;
		const float deltaMove = dx * dx + dz * dz;

		if (deltaMove > 1e-6)
		{
			const float yaw = std::atan2(-dx, -dz);
			actionDelta->hasYaw = true;
			actionDelta->yaw = yaw;
		}
	}

	if (finished)
		ForceNextSegment(actionMove, segments);
}