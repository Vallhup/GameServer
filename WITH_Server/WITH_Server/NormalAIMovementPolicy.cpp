#include "pch.h"
#include "NormalAIMovementPolicy.h"

#include "IAIState.h"
#include "TransformHelper.h"

void NormalAIMovementPolicy::BuildChaseIntent(AIContext& ctx)
{
	XMFLOAT3 targetPos;
	if (!TryGetCurrentTargetPosition(ctx, targetPos))
		return;

	ctx.command->hasMove = true;
	ctx.command->wantsRun = true;

	const XMVECTOR vFrom = XMLoadFloat3(&ctx.selfTr->position);
	const XMVECTOR vTo = XMLoadFloat3(&targetPos);

	XMVECTOR dir = XMVectorSubtract(vTo, vFrom);
	if (XMVectorGetX(XMVector3LengthSq(dir)) > 1e-12f)
	{
		dir = XMVector3Normalize(dir);
	}

	else
	{
		dir = XMVectorZero();
	}

	XMStoreFloat3(&ctx.command->moveDir, dir);
}

void NormalAIMovementPolicy::BuildCombatIntent(AIContext& ctx)
{
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

	ctx.command->hasMove = true;
	ctx.command->wantsRun = false;

	if (dist > tooFar)
	{
		// 접근
		ctx.command->moveDir = toTarget;
	}

	else if (dist < tooClose)
	{
		// 후퇴
		ctx.command->moveDir = { -toTarget.x, 0.0f, -toTarget.z };
	}

	else
	{
		// 간격 유지
		ctx.command->hasMove = false;
		ctx.command->moveDir = { 0, 0, 0 };
	}
}

void NormalAIMovementPolicy::BuildSearchIntent(AIContext& ctx)
{
	if (!ctx.blackboard->hasLastKnownTargetPosition) return;

	XMVECTOR dirV = TransformHelper::Direction(
		ctx.selfTr->position,
		ctx.blackboard->lastKnownTargetPosition);

	const float dirLengthSq = XMVectorGetX(XMVector3LengthSq(dirV));
	if (dirLengthSq > 1e-6f)
	{
		ctx.command->hasMove = true;
		ctx.command->wantsRun = false;

		XMStoreFloat3(&ctx.command->moveDir, dirV);
	}
}

bool NormalAIMovementPolicy::TryGetCurrentTargetPosition(
	AIContext& ctx,
	XMFLOAT3& outTargetPos) const
{
	if (ctx.blackboard->currentTarget.IsNull()) return false;

	const auto* targetTr =
		ctx.sysCtx->ecs.GetComponent<WorldTransformComp>(ctx.blackboard->currentTarget);
	if (!targetTr) return false;

	outTargetPos = targetTr->position;
	return true;
}