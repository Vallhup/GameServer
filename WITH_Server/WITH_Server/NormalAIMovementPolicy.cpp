#include "pch.h"
#include "NormalAIMovementPolicy.h"

#include "IAIState.h"
#include "TransformHelper.h"

#include <cmath>

void NormalAIMovementPolicy::BuildChaseIntent(AIContext& ctx)
{
	XMFLOAT3 targetPos;
	if (!TryGetCurrentTargetPosition(ctx, targetPos))
		return;

	XMVECTOR dir = TransformHelper::Direction(ctx.selfTr->position, targetPos);
	XMStoreFloat3(&ctx.command->moveDir, dir);

	const bool hasMove = !XMVector3Equal(dir, XMVectorZero());
	ctx.command->hasMove = hasMove;
	ctx.command->wantsRun = hasMove;
}

void NormalAIMovementPolicy::BuildCombatIntent(AIContext& ctx)
{
	XMFLOAT3 targetPos;
	if (!TryGetCurrentTargetPosition(ctx, targetPos) ||
		ctx.command == nullptr ||
		ctx.selfTr == nullptr ||
		ctx.perception == nullptr ||
		ctx.perceptionTuning == nullptr)
	{
		return;
	}

	XMFLOAT3 toTarget{ 0.0f, 0.0f, 0.0f };
	const XMVECTOR toTargetV =
		TransformHelper::Direction(ctx.selfTr->position, targetPos);
	XMStoreFloat3(&toTarget, toTargetV);

	const float dirLengthSq =
		toTarget.x * toTarget.x +
		toTarget.z * toTarget.z;
	if (dirLengthSq <= 1.0e-6f)
	{
		ctx.command->hasMove = false;
		ctx.command->wantsRun = false;
		ctx.command->moveDir = { 0.0f, 0.0f, 0.0f };
		return;
	}

	const double dist = ctx.perception->distanceToTarget;
	const double desired = ctx.perceptionTuning->attackRange * 0.72;
	const double tooClose = desired * 0.35;
	const double tooFar = desired * 1.01;

	ctx.command->hasMove = false;
	ctx.command->wantsRun = false;
	ctx.command->moveDir = { 0.0f, 0.0f, 0.0f };

	if (dist > tooFar)
	{
		ctx.command->hasMove = true;
		ctx.command->moveDir = toTarget;
		return;
	}

	if (dist < tooClose)
	{
		ctx.command->hasMove = true;
		ctx.command->moveDir = { -toTarget.x, 0.0f, -toTarget.z };
		return;
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
