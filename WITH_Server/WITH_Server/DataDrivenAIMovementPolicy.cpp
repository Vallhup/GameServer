#include "pch.h"
#include "DataDrivenAIMovementPolicy.h"

#include "AICombatRangePolicy.h"
#include "AIMovementDistanceResolver.h"
#include "AIMovementPolicyUtil.h"
#include "ECS/Components/GameplayAIComponents.h"
#include "System.h"
#include "TransformHelper.h"

namespace
{
	constexpr float kChaseContactPaddingXZ = 0.05f;

	bool HasReachedChaseBodyContact(
		const AIContext& ctx,
		const DirectX::XMFLOAT3& targetPosition) noexcept
	{
		if (ctx.sysCtx == nullptr ||
			ctx.selfTr == nullptr ||
			ctx.blackboard == nullptr ||
			ctx.blackboard->currentTarget.IsNull())
		{
			return false;
		}

		const BodyCollisionShapeComp* selfShape =
			ctx.sysCtx->ecs.GetComponent<BodyCollisionShapeComp>(ctx.self);
		const BodyCollisionShapeComp* targetShape =
			ctx.sysCtx->ecs.GetComponent<BodyCollisionShapeComp>(
				ctx.blackboard->currentTarget);
		if (selfShape == nullptr ||
			targetShape == nullptr ||
			!selfShape->blocksBodyOverlap ||
			!targetShape->blocksBodyOverlap ||
			selfShape->pushability == BodyPushability::None ||
			targetShape->pushability == BodyPushability::None)
		{
			return false;
		}

		const float contactDistance =
			std::max(selfShape->bodyRadiusXZ, 0.0f) +
			std::max(targetShape->bodyRadiusXZ, 0.0f) +
			kChaseContactPaddingXZ;
		const float dx = targetPosition.x - ctx.selfTr->position.x;
		const float dz = targetPosition.z - ctx.selfTr->position.z;
		return dx * dx + dz * dz <= contactDistance * contactDistance;
	}
}

void DataDrivenAIMovementPolicy::BuildChaseIntent(AIContext& ctx) const
{
	const AIMovementProfileDef* profile = SelectMovementProfile(ctx);
	if (ctx.intent != nullptr && profile != nullptr)
	{
		ctx.intent->lockFacingToLookTarget = profile->lockFacingToTarget;
	}

	DirectX::XMFLOAT3 targetPos{};
	if (!AIMovementPolicyUtil::TryGetCurrentTargetPosition(ctx, targetPos))
		return;

	if (HasReachedChaseBodyContact(ctx, targetPos))
	{
		AIMovementPolicyUtil::ClearMove(ctx);
		return;
	}

	const bool wantsRun =
		profile == nullptr ||
		profile->farBehavior == AIMovementBehavior::RunApproach;

	AIMovementPolicyUtil::BuildDestinationIntent(ctx, targetPos, wantsRun);
}

void DataDrivenAIMovementPolicy::BuildCombatIntent(AIContext& ctx) const
{
	if (ctx.intent == nullptr || ctx.perception == nullptr)
		return;

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
		AIMovementDistanceResolver::ResolveBehavior(
			*profile,
			ctx.perception->distanceToTarget,
			ctx.movementRuntime);
	const bool canSelectCombatAction =
		AICombatRangePolicy::CanSelectCombatAction(ctx);
	const AIMovementBehavior resolvedBehavior =
		behavior == AIMovementBehavior::Hold &&
			!canSelectCombatAction &&
			ctx.perception->distanceToTarget > profile->veryCloseDistance
			? AIMovementBehavior::Approach
			: behavior;

	ApplyMovementBehavior(ctx, *profile, toTarget, resolvedBehavior);
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

void DataDrivenAIMovementPolicy::BuildReturnHomeIntent(
	AIContext& ctx) const
{
	if (ctx.blackboard == nullptr ||
		!ctx.blackboard->hasHomePosition)
	{
		return;
	}

	AIMovementPolicyUtil::BuildDestinationIntent(
		ctx,
		ctx.blackboard->homePosition,
		false);
}

const AIMovementProfileDef* DataDrivenAIMovementPolicy::SelectMovementProfile(
	const AIContext& ctx) noexcept
{
	if (ctx.behaviorProfile == nullptr)
		return nullptr;

	uint8_t phase{ 1 };
	if (ctx.sysCtx != nullptr)
	{
		if (const AIPhaseRuntimeComp* phaseComp =
			ctx.sysCtx->ecs.GetComponent<AIPhaseRuntimeComp>(ctx.self))
		{
			phase = std::max<uint8_t>(1, phaseComp->currentPhase);
		}
	}

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

	const DirectX::XMVECTOR toTargetV =
		TransformHelper::Direction(ctx.selfTr->position, targetPos);
	DirectX::XMStoreFloat3(&outDirection, toTargetV);

	const float lengthSq =
		outDirection.x * outDirection.x +
		outDirection.z * outDirection.z;
	if (lengthSq <= kMoveEpsilonSq)
		return false;

	const float invLength = 1.0f / std::sqrt(lengthSq);
	outDirection.x *= invLength;
	outDirection.y = 0.0f;
	outDirection.z *= invLength;
	return true;
}

int DataDrivenAIMovementPolicy::ResolveStrafeSign(
	AIContext& ctx,
	const AIMovementProfileDef& profile)
{
	AIMovementRuntimeComp* runtime = ctx.movementRuntime;
	if (runtime == nullptr && ctx.sysCtx != nullptr)
	{
		runtime =
			ctx.sysCtx->ecs.GetMutableComponent<AIMovementRuntimeComp>(
				ctx.self);
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
