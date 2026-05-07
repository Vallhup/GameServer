#include "pch.h"
#include "DataDrivenAIMovementPolicy.h"

#include "AIBehaviorDef.h"
#include "AIMovementPolicyUtil.h"
#include "IAIState.h"
#include "System.h"
#include "TransformHelper.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr float kMoveEpsilonSq = 1.0e-6f;

	uint8_t ResolvePhase(const AIContext& ctx) noexcept
	{
		if (ctx.sysCtx == nullptr)
			return 1;

		const AIPhaseRuntimeComp* phase =
			ctx.sysCtx->ecs.GetComponent<AIPhaseRuntimeComp>(ctx.self);
		return phase ? std::max<uint8_t>(1, phase->currentPhase) : 1;
	}

	const AIMovementProfileDef* SelectMovementProfile(
		const AIContext& ctx) noexcept
	{
		if (ctx.behaviorProfile == nullptr)
			return nullptr;

		const uint8_t phase = ResolvePhase(ctx);
		for (const AIMovementProfileDef& profile :
			ctx.behaviorProfile->movementProfiles)
		{
			if (phase >= profile.phaseMin && phase <= profile.phaseMax)
				return &profile;
		}

		return ctx.behaviorProfile->movementProfiles.empty()
			? nullptr
			: &ctx.behaviorProfile->movementProfiles.front();
	}

	bool TryBuildTargetDirection(
		AIContext& ctx,
		DirectX::XMFLOAT3& outToTarget)
	{
		if (ctx.selfTr == nullptr)
			return false;

		DirectX::XMFLOAT3 targetPos{};
		if (!AIMovementPolicyUtil::TryGetCurrentTargetPosition(ctx, targetPos))
			return false;

		const DirectX::XMVECTOR toTargetV =
			TransformHelper::Direction(ctx.selfTr->position, targetPos);
		DirectX::XMStoreFloat3(&outToTarget, toTargetV);

		const float lengthSq =
			outToTarget.x * outToTarget.x +
			outToTarget.z * outToTarget.z;
		if (lengthSq <= kMoveEpsilonSq)
			return false;

		const float invLength = 1.0f / std::sqrt(lengthSq);
		outToTarget.x *= invLength;
		outToTarget.y = 0.0f;
		outToTarget.z *= invLength;
		return true;
	}

	AIMovementBehavior ResolveCombatBehavior(
		const AIMovementProfileDef& profile,
		double distance) noexcept
	{
		if (distance <= profile.veryCloseDistance)
			return profile.veryCloseBehavior;

		if (profile.preferredMinDistance <= profile.preferredMaxDistance &&
			distance >= profile.preferredMinDistance &&
			distance <= profile.preferredMaxDistance)
		{
			return profile.preferredBehavior;
		}

		if (distance <= profile.closeDistance)
			return profile.closeBehavior;

		if (distance <= profile.midDistance)
			return profile.midBehavior;

		return profile.farBehavior;
	}

	int ResolveStrafeSign(
		AIContext& ctx,
		const AIMovementProfileDef& profile,
		int fixedSign)
	{
		if (fixedSign != 0)
			return fixedSign;

		AIMovementRuntimeComp* runtime = ctx.movementRuntime;
		if (runtime == nullptr && ctx.sysCtx != nullptr)
		{
			runtime =
				ctx.sysCtx->ecs.GetMutableComponent<AIMovementRuntimeComp>(ctx.self);
		}

		if (runtime == nullptr)
		{
			return ctx.actionRuntime != nullptr &&
				(ctx.actionRuntime->actionSequence & 1u) != 0u
				? -1
				: 1;
		}

		const float dt = ctx.sysCtx != nullptr
			? static_cast<float>(ctx.sysCtx->dtSec)
			: 0.0f;
		runtime->strafeTimeLeftSec =
			std::max(0.0f, runtime->strafeTimeLeftSec - dt);

		if (runtime->strafeTimeLeftSec <= 0.0f)
		{
			runtime->strafeSign = -runtime->strafeSign;
			if (runtime->strafeSign == 0)
				runtime->strafeSign = 1;

			const uint32_t sequence = ctx.actionRuntime
				? ctx.actionRuntime->actionSequence
				: 0u;
			const uint32_t mixed =
				static_cast<uint32_t>(ctx.self.id) * 1103515245u +
				sequence * 12345u;
			const float t = static_cast<float>(mixed % 1000u) / 999.0f;
			runtime->strafeTimeLeftSec =
				profile.strafeMinSec +
				(profile.strafeMaxSec - profile.strafeMinSec) * t;
		}

		return runtime->strafeSign;
	}

	void ApplyBehavior(
		AIContext& ctx,
		const AIMovementProfileDef& profile,
		const DirectX::XMFLOAT3& toTarget,
		AIMovementBehavior behavior)
	{
		switch (behavior) {
		case AIMovementBehavior::Approach:
			AIMovementPolicyUtil::StoreMoveDirection(ctx, toTarget, false);
			break;
		case AIMovementBehavior::RunApproach:
			AIMovementPolicyUtil::StoreMoveDirection(ctx, toTarget, true);
			break;
		case AIMovementBehavior::Retreat:
			AIMovementPolicyUtil::StoreMoveDirection(
				ctx,
				{ -toTarget.x, 0.0f, -toTarget.z },
				false);
			break;
		case AIMovementBehavior::Strafe:
		{
			const int sign = ResolveStrafeSign(ctx, profile, 0);
			AIMovementPolicyUtil::StoreMoveDirection(
				ctx,
				{
					static_cast<float>(sign) * toTarget.z,
					0.0f,
					static_cast<float>(-sign) * toTarget.x
				},
				false);
			break;
		}
		case AIMovementBehavior::CircleLeft:
			AIMovementPolicyUtil::StoreMoveDirection(
				ctx,
				{ -toTarget.z, 0.0f, toTarget.x },
				false);
			break;
		case AIMovementBehavior::CircleRight:
			AIMovementPolicyUtil::StoreMoveDirection(
				ctx,
				{ toTarget.z, 0.0f, -toTarget.x },
				false);
			break;
		case AIMovementBehavior::None:
		case AIMovementBehavior::Hold:
		default:
			AIMovementPolicyUtil::ClearMove(ctx);
			break;
		}
	}
}

void DataDrivenAIMovementPolicy::BuildChaseIntent(AIContext& ctx) const
{
	DirectX::XMFLOAT3 targetPos{};
	if (!AIMovementPolicyUtil::TryGetCurrentTargetPosition(ctx, targetPos))
		return;

	const AIMovementProfileDef* profile = SelectMovementProfile(ctx);
	const bool wantsRun =
		profile == nullptr ||
		profile->farBehavior == AIMovementBehavior::RunApproach;
	AIMovementPolicyUtil::BuildDestinationIntent(ctx, targetPos, wantsRun);
}

void DataDrivenAIMovementPolicy::BuildCombatIntent(AIContext& ctx) const
{
	if (ctx.intent == nullptr ||
		ctx.perception == nullptr)
	{
		return;
	}

	const AIMovementProfileDef* profile = SelectMovementProfile(ctx);
	if (profile == nullptr)
	{
		AIMovementPolicyUtil::ClearMove(ctx);
		return;
	}

	ctx.intent->lockFacingToLookTarget = profile->lockFacingToTarget;

	if (ctx.actionRuntime != nullptr &&
		ctx.actionRuntime->movementLockSec > 0.0f)
	{
		AIMovementPolicyUtil::ClearMove(ctx);
		return;
	}

	DirectX::XMFLOAT3 toTarget{};
	if (!TryBuildTargetDirection(ctx, toTarget))
	{
		AIMovementPolicyUtil::ClearMove(ctx);
		return;
	}

	const AIMovementBehavior behavior =
		ResolveCombatBehavior(*profile, ctx.perception->distanceToTarget);
	ApplyBehavior(ctx, *profile, toTarget, behavior);
}

void DataDrivenAIMovementPolicy::BuildSearchIntent(AIContext& ctx) const
{
	if (ctx.blackboard == nullptr ||
		!ctx.blackboard->hasLastKnownTargetPosition)
	{
		return;
	}

	AIMovementPolicyUtil::BuildDestinationIntent(
		ctx,
		ctx.blackboard->lastKnownTargetPosition,
		false);
}

void DataDrivenAIMovementPolicy::BuildReturnHomeIntent(AIContext& ctx) const
{
	if (ctx.blackboard == nullptr || !ctx.blackboard->hasHomePosition)
		return;

	AIMovementPolicyUtil::BuildDestinationIntent(
		ctx,
		ctx.blackboard->homePosition,
		false);
}
