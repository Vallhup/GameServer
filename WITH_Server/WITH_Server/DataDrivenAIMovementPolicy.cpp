#include "pch.h"
#include "DataDrivenAIMovementPolicy.h"

#include "AIMovementPolicyUtil.h"
#include "ECS/Components/GameplayAIComponents.h"
#include "System.h"
#include "TransformHelper.h"

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
	if (!TryComputeDirectionToTarget(ctx, toTarget))
	{
		AIMovementPolicyUtil::ClearMove(ctx);
		return;
	}

	const AIMovementBehavior behavior =
		ResolveCombatBehavior(*profile, ctx.perception->distanceToTarget);

	ApplyMovementBehavior(ctx, *profile, toTarget, behavior);
}

void DataDrivenAIMovementPolicy::BuildSearchIntent(AIContext& ctx) const
{
	if (ctx.blackboard == nullptr ||
		!ctx.blackboard->hasLastKnownTargetPosition)
	{
		return;
	}

	AIMovementPolicyUtil::BuildDestinationIntent(
		ctx, ctx.blackboard->lastKnownTargetPosition, false);
}

void DataDrivenAIMovementPolicy::BuildReturnHomeIntent(AIContext& ctx) const
{
	if (ctx.blackboard == nullptr ||
		!ctx.blackboard->hasHomePosition)
	{
		return;
	}

	AIMovementPolicyUtil::BuildDestinationIntent(
		ctx, ctx.blackboard->homePosition, false);
}

const AIMovementProfileDef* DataDrivenAIMovementPolicy::SelectMovementProfile(
	const AIContext& ctx) noexcept
{
	if (ctx.behaviorProfile == nullptr)
		return nullptr;

	uint8_t phase{ 1 };

	// 1. phase 계산
	{
		if (ctx.sysCtx != nullptr)
		{
			if (const AIPhaseRuntimeComp* phaseComp =
				ctx.sysCtx->ecs.GetComponent<AIPhaseRuntimeComp>(ctx.self))
			{
				phase = std::max<uint8_t>(1, phaseComp->currentPhase);
			}
		}
	}

	// 2. phase 범위 안의 profile 검색
	{
		for (const AIMovementProfileDef& profile :
			ctx.behaviorProfile->movementProfiles)
		{
			if (phase >= profile.phaseMin && phase <= profile.phaseMax)
				return &profile;
		}
	}

	// 3. fallback: 첫 번째 profile
	{
		if (ctx.behaviorProfile->movementProfiles.empty())
			return nullptr;

		return &ctx.behaviorProfile->movementProfiles.front();
	}
}

bool DataDrivenAIMovementPolicy::TryComputeDirectionToTarget(
	AIContext& ctx,
	DirectX::XMFLOAT3& outDirection)
{
	constexpr float kMoveEpsilonSq = 1.0e-6f;

	outDirection = DirectX::XMFLOAT3{ 0.0f, 0.0f, 0.0f };

	if (ctx.selfTr == nullptr)
		return false;

	DirectX::XMFLOAT3 targetPos{};
	if (!AIMovementPolicyUtil::TryGetCurrentTargetPosition(ctx, targetPos))
		return false;

	// 1. self -> target 방향 벡터 계산
	{
		const DirectX::XMVECTOR toTargetV =
			TransformHelper::Direction(ctx.selfTr->position, targetPos);

		DirectX::XMStoreFloat3(&outDirection, toTargetV);
	}

	// 2. XZ 평면 거리 검사 및 정규화
	{
		const float lengthSq =
			outDirection.x * outDirection.x +
			outDirection.z * outDirection.z;

		if (lengthSq <= kMoveEpsilonSq)
			return false;

		const float invLength = 1.0f / std::sqrt(lengthSq);

		outDirection.x *= invLength;
		outDirection.y  = 0.0f;
		outDirection.z *= invLength;
	}

	return true;
}

AIMovementBehavior DataDrivenAIMovementPolicy::ResolveCombatBehavior(
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

int DataDrivenAIMovementPolicy::ResolveStrafeSign(
	AIContext& ctx,
	const AIMovementProfileDef& profile)
{
	AIMovementRuntimeComp* runtime = ctx.movementRuntime;

	// 1. runtime 컴포넌트 확보
	{
		if (runtime == nullptr && ctx.sysCtx != nullptr)
		{
			runtime =
				ctx.sysCtx->ecs.GetMutableComponent<AIMovementRuntimeComp>(
					ctx.self);
		}
	}

	// 2. runtime 부재 시 fallback (actionSequence parity 기반)
	{
		if (runtime == nullptr)
		{
			return ctx.actionRuntime != nullptr &&
				(ctx.actionRuntime->actionSequence & 1u) != 0u
				? -1
				: 1;
		}
	}

	// 3. strafe 잔여 시간 차감
	{
		const float dt = ctx.sysCtx != nullptr
			? static_cast<float>(ctx.sysCtx->dtSec)
			: 0.0f;

		runtime->strafeTimeLeftSec =
			std::max(0.0f, runtime->strafeTimeLeftSec - dt);
	}

	// 4. 잔여 시간 소진 시 부호 전환 및 다음 주기 길이 추첨
	{
		if (runtime->strafeTimeLeftSec <= 0.0f)
		{
			runtime->strafeSign = -runtime->strafeSign;
			if (runtime->strafeSign == 0)
				runtime->strafeSign = 1;

			const uint32_t sequence = ctx.actionRuntime != nullptr
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
	}

	return runtime->strafeSign;
}

void DataDrivenAIMovementPolicy::ApplyMovementBehavior(
	AIContext& ctx,
	const AIMovementProfileDef& profile,
	const DirectX::XMFLOAT3& toTarget,
	AIMovementBehavior behavior)
{
	switch (behavior) {
	case AIMovementBehavior::Approach:
	{
		AIMovementPolicyUtil::StoreMoveDirection(ctx, toTarget, false);
		break;
	}
	case AIMovementBehavior::RunApproach:
	{
		AIMovementPolicyUtil::StoreMoveDirection(ctx, toTarget, true);
		break;
	}
	case AIMovementBehavior::Retreat:
	{
		AIMovementPolicyUtil::StoreMoveDirection(
			ctx,
			{ -toTarget.x, 0.0f, -toTarget.z },
			false);
		break;
	}
	case AIMovementBehavior::Strafe:
	{
		const int sign = ResolveStrafeSign(ctx, profile);

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
	{
		AIMovementPolicyUtil::StoreMoveDirection(
			ctx,
			{ -toTarget.z, 0.0f, toTarget.x },
			false);
		break;
	}
	case AIMovementBehavior::CircleRight:
	{
		AIMovementPolicyUtil::StoreMoveDirection(
			ctx,
			{ toTarget.z, 0.0f, -toTarget.x },
			false);
		break;
	}
	case AIMovementBehavior::None:
	case AIMovementBehavior::Hold:
	default:
	{
		AIMovementPolicyUtil::ClearMove(ctx);
		break;
	}
	}
}
