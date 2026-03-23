#include "pch.h"
#include "NormalAIMovementPolicy.h"

#include "IAIState.h"

void NormalAIMovementPolicy::BuildChaseIntent(AIContext& ctx)
{
	if (!ctx.IsValidContext()) return;

	XMFLOAT3 targetPos;
	if (!TryGetCurrentTargetPosition(ctx, targetPos))
		return;

	ctx.intent->hasMove = true;
	ctx.intent->moveRun = true;

	XMVECTOR moveDirV = TransformHelper::Direction(ctx.selfTr->position, targetPos);
	XMStoreFloat3(&ctx.intent->moveDir, moveDirV);
}

void NormalAIMovementPolicy::BuildCombatIntent(AIContext& ctx)
{
	if (!ctx.IsValidContext()) return;

	XMFLOAT3 targetPos;
	if (!TryGetCurrentTargetPosition(ctx, targetPos))
		return;

	XMFLOAT3 toTarget{ 0, 0, 0 };
	XMVECTOR toTargetV = TransformHelper::Direction(ctx.selfTr->position, targetPos);
	XMStoreFloat3(&toTarget, toTargetV);

	const double dist = ctx.perception->distanceToTarget;
	const double desired = ctx.perceptionTuning->attackRange * 0.9;
	const double tooClose = desired * 0.65;
	const double tooFar = desired * 1.10;

	ctx.intent->hasMove = true;
	ctx.intent->moveRun = false;

	if (dist > tooFar)
	{
		// 접근
		ctx.intent->moveDir = toTarget;
	}

	else if (dist < tooClose)
	{
		// 후퇴
		ctx.intent->moveDir = { -toTarget.x, 0.0f, -toTarget.z };
	}

	else
	{
		// 간격 유지: 일단 정지
		// TEMP : 추후 Strafe 넣을 수 있음
		ctx.intent->hasMove = false;
		ctx.intent->moveDir = { 0, 0, 0 };
	}
}

void NormalAIMovementPolicy::BuildSearchIntent(AIContext& ctx)
{
	if (!ctx.IsValidContext()) return;
	if (!ctx.blackboard->hasLastKnownTargetPosition) return;

	XMVECTOR dirV = TransformHelper::Direction(
		ctx.selfTr->position,
		ctx.blackboard->lastKnownTargetPosition);

	const float dirLengthSq = XMVectorGetX(XMVector3LengthSq(dirV));
	if (dirLengthSq > 1e-6f)
	{
		ctx.intent->hasMove = true;
		ctx.intent->moveRun = false;

		XMStoreFloat3(&ctx.intent->moveDir, dirV);
	}
}

bool NormalAIMovementPolicy::TryGetCurrentTargetPosition(
	AIContext& ctx, 
	XMFLOAT3& outTargetPos) const
{
	if (!ctx.IsValidContext()) return false;
	if (ctx.blackboard->currentTarget.IsNull()) return false;

	const ECS& ecs = ctx.runtime->GetECS();
	const auto* targetTr =
		ecs.GetStorage<Transform>().GetComponent(ctx.blackboard->currentTarget);

	if (!targetTr) return false;

	outTargetPos = targetTr->position;
	return true;
}
