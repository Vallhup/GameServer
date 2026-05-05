#include "pch.h"
#include "BossAIMovementPolicy.h"

#include "IAIState.h"
#include "System.h"
#include "TransformHelper.h"
#include "ECS/Components/GameplayAIComponents.h"

namespace
{
	static constexpr float kMoveEpsilonSq = 1.0e-6f;
	static constexpr double kVeryCloseDistance = 2.0;
	static constexpr double kCloseDistance = 5.8;
	static constexpr double kMidDistance = 7.5;
	static constexpr double kPhase2PreferredMinDistance = 3.2;
	static constexpr double kPhase2PreferredMaxDistance = 6.8;
	static constexpr float kMinStrafeDurationSec = 0.7f;
	static constexpr float kMaxStrafeDurationSec = 1.5f;

	bool TryGetCurrentTargetPosition(
		AIContext& ctx,
		DirectX::XMFLOAT3& outTargetPos)
	{
		if (ctx.blackboard == nullptr ||
			ctx.blackboard->currentTarget.IsNull() ||
			ctx.sysCtx == nullptr)
		{
			return false;
		}

		const auto* targetTr =
			ctx.sysCtx->ecs.GetComponent<WorldTransformComp>(
				ctx.blackboard->currentTarget);
		if (targetTr == nullptr)
		{
			return false;
		}

		outTargetPos = targetTr->position;
		return true;
	}

	void ClearMove(AIContext& ctx)
	{
		if (ctx.command == nullptr)
		{
			return;
		}

		ctx.command->hasMove = false;
		ctx.command->wantsRun = false;
		ctx.command->moveDir = { 0.0f, 0.0f, 0.0f };
	}

	bool StoreMoveDirection(
		AIContext& ctx,
		DirectX::XMFLOAT3 moveDir,
		bool wantsRun)
	{
		if (ctx.command == nullptr)
		{
			return false;
		}

		const float lengthSq =
			moveDir.x * moveDir.x +
			moveDir.z * moveDir.z;
		if (lengthSq <= kMoveEpsilonSq)
		{
			ClearMove(ctx);
			return false;
		}

		const float invLength = 1.0f / std::sqrt(lengthSq);
		moveDir.x *= invLength;
		moveDir.y = 0.0f;
		moveDir.z *= invLength;

		ctx.command->hasMove = true;
		ctx.command->wantsRun = wantsRun;
		ctx.command->moveDir = moveDir;
		return true;
	}

	int GetStrafeSign(const AIContext& ctx) noexcept
	{
		if (ctx.blackboard == nullptr)
		{
			return 1;
		}

		return (ctx.blackboard->combatActionSequence & 1u) == 0u ? 1 : -1;
	}

	uint8_t ResolvePhase(const AIContext& ctx) noexcept
	{
		if (ctx.sysCtx == nullptr)
		{
			return 1;
		}

		const BossPhaseStateComp* phase =
			ctx.sysCtx->ecs.GetComponent<BossPhaseStateComp>(ctx.self);
		return phase ? std::max<uint8_t>(1, phase->currentPhase) : 1;
	}

	BossPatternRuntimeComp* TryGetPatternRuntime(AIContext& ctx) noexcept
	{
		return ctx.sysCtx
			? ctx.sysCtx->ecs.GetMutableComponent<BossPatternRuntimeComp>(ctx.self)
			: nullptr;
	}

	void StoreStrafeDirection(
		AIContext& ctx,
		const DirectX::XMFLOAT3& toTarget,
		int sign)
	{
		StoreMoveDirection(
			ctx,
			{
				static_cast<float>(sign) * toTarget.z,
				0.0f,
				static_cast<float>(-sign) * toTarget.x
			},
			false);
	}

	float PickStrafeDuration(const AIContext& ctx) noexcept
	{
		const uint32_t sequence = ctx.blackboard
			? ctx.blackboard->combatActionSequence
			: 0u;
		const uint32_t mixed =
			static_cast<uint32_t>(ctx.self.id) * 1103515245u +
			sequence * 12345u;
		const float t = static_cast<float>(mixed % 1000u) / 999.0f;
		return kMinStrafeDurationSec +
			(kMaxStrafeDurationSec - kMinStrafeDurationSec) * t;
	}
}

void BossAIMovementPolicy::BuildCombatIntent(AIContext& ctx) const
{
	if (ctx.command == nullptr ||
		ctx.selfTr == nullptr ||
		ctx.perception == nullptr)
	{
		return;
	}

	ctx.command->lockFacingToLookTarget = true;

	if (BossPatternRuntimeComp* runtime = TryGetPatternRuntime(ctx);
		runtime != nullptr && runtime->phaseTransitionLockSec > 0.0f)
	{
		ClearMove(ctx);
		return;
	}

	DirectX::XMFLOAT3 targetPos{};
	if (!TryGetCurrentTargetPosition(ctx, targetPos))
	{
		ClearMove(ctx);
		return;
	}

	DirectX::XMFLOAT3 toTarget{};
	const DirectX::XMVECTOR toTargetV =
		TransformHelper::Direction(ctx.selfTr->position, targetPos);
	DirectX::XMStoreFloat3(&toTarget, toTargetV);

	const float lengthSq =
		toTarget.x * toTarget.x +
		toTarget.z * toTarget.z;
	if (lengthSq <= kMoveEpsilonSq)
	{
		ClearMove(ctx);
		return;
	}

	const float invLength = 1.0f / std::sqrt(lengthSq);
	toTarget.x *= invLength;
	toTarget.y = 0.0f;
	toTarget.z *= invLength;

	const double dist = ctx.perception->distanceToTarget;
	const uint8_t phase = ResolvePhase(ctx);

	if (phase <= 1)
	{
		if (dist <= kVeryCloseDistance)
		{
			StoreMoveDirection(
				ctx,
				{ -toTarget.x, 0.0f, -toTarget.z },
				false);
			return;
		}

		if (dist <= kCloseDistance)
		{
			ClearMove(ctx);
			return;
		}

		StoreMoveDirection(ctx, toTarget, dist > kMidDistance);
		return;
	}

	BossPatternRuntimeComp* runtime = TryGetPatternRuntime(ctx);

	if (dist < kPhase2PreferredMinDistance)
	{
		StoreMoveDirection(
			ctx,
			{ -toTarget.x, 0.0f, -toTarget.z },
			false);
		return;
	}

	if (dist <= kPhase2PreferredMaxDistance)
	{
		int sign = GetStrafeSign(ctx);
		if (runtime != nullptr)
		{
			if (runtime->strafeTimeLeftSec <= 0.0f)
			{
				runtime->strafeSign = -runtime->strafeSign;
				if (runtime->strafeSign == 0)
				{
					runtime->strafeSign = sign;
				}
				runtime->strafeTimeLeftSec = PickStrafeDuration(ctx);
			}
			sign = runtime->strafeSign;
		}

		StoreStrafeDirection(ctx, toTarget, sign);
		return;
	}

	if (dist <= kMidDistance)
	{
		StoreMoveDirection(ctx, toTarget, false);
		return;
	}

	StoreMoveDirection(ctx, toTarget, true);
}
