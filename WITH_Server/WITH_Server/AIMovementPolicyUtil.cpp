#include "pch.h"
#include "AIMovementPolicyUtil.h"

#include "IAIState.h"
#include "NavMeshRuntime.h"
#include "System.h"
#include "TransformHelper.h"
#include "WorldDef.h"

#include "Library/Recast/Recast.h"
#include "Library/Detour/DetourNavMesh.h"
#include "Library/Detour/DetourNavMeshQuery.h"

#include <cmath>

namespace
{
	constexpr float kMoveEpsilonSq = 1.0e-6f;
	constexpr double kPathRecomputeIntervalSec = 0.35;
	constexpr double kDestinationChangeEpsilonSq = 0.25;
	constexpr double kPathCornerArriveRangeSq = 0.25;

	dtQueryFilter BuildQueryFilter(const NavigationProfileDef* profile)
	{
		dtQueryFilter filter;
		if (profile != nullptr)
		{
			filter.setIncludeFlags(profile->queryFilter.includeFlags);
			filter.setExcludeFlags(profile->queryFilter.excludeFlags);
			filter.setAreaCost(RC_WALKABLE_AREA, profile->queryFilter.walkableAreaCost);
		}
		return filter;
	}

	bool TryFindNearestPolyPoint(
		dtNavMeshQuery* query,
		const float* center,
		const float* extents,
		const dtQueryFilter& filter,
		dtPolyRef& outRef,
		float* outPoint)
	{
		outRef = 0;
		const dtStatus status =
			query->findNearestPoly(center, extents, &filter, &outRef, outPoint);
		return !dtStatusFailed(status) && outRef != 0;
	}

	double DistanceSqXZ(
		const DirectX::XMFLOAT3& lhs,
		const DirectX::XMFLOAT3& rhs) noexcept
	{
		const double dx =
			static_cast<double>(lhs.x) - static_cast<double>(rhs.x);
		const double dz =
			static_cast<double>(lhs.z) - static_cast<double>(rhs.z);
		return dx * dx + dz * dz;
	}

	bool TryStoreDirectionTo(
		AIContext& ctx,
		const DirectX::XMFLOAT3& destination,
		bool wantsRun)
	{
		if (ctx.intent == nullptr || ctx.selfTr == nullptr)
			return false;

		DirectX::XMVECTOR dir =
			TransformHelper::Direction(ctx.selfTr->position, destination);
		const float dirLengthSq =
			DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(dir));
		if (dirLengthSq <= kMoveEpsilonSq)
		{
			AIMovementPolicyUtil::ClearMove(ctx);
			return false;
		}

		DirectX::XMStoreFloat3(&ctx.intent->moveDir, dir);
		ctx.intent->hasMove = true;
		ctx.intent->wantsRun = wantsRun;
		return true;
	}

	bool TryBuildPathCorner(
		AIContext& ctx,
		const DirectX::XMFLOAT3& destination,
		DirectX::XMFLOAT3& outCorner)
	{
		const INavMeshProvider* navProvider =
			ctx.sysCtx ? ctx.sysCtx->services.navMeshProvider : nullptr;
		const NavMeshRuntime* navMesh =
			navProvider ? navProvider->GetNavMeshRuntime() : nullptr;
		const NavigationProfileDef* profile =
			navProvider ? navProvider->GetNavigationProfile() : nullptr;

		if (navMesh == nullptr || !navMesh->IsReady())
			return false;

		dtNavMeshQuery* query = navMesh->GetQuery();
		if (query == nullptr)
			return false;

		dtQueryFilter filter = BuildQueryFilter(profile);
		const float extentXZ = profile ? profile->nearestPolyExtentXZ : 2.0f;
		const float extentY = profile ? profile->nearestPolyExtentY : 4.0f;
		const float extents[3] = { extentXZ, extentY, extentXZ };

		const float startPos[3] =
		{
			ctx.selfTr->position.x,
			ctx.selfTr->position.y,
			ctx.selfTr->position.z
		};
		const float endPos[3] =
		{
			destination.x,
			destination.y,
			destination.z
		};

		dtPolyRef startRef = 0;
		dtPolyRef endRef = 0;
		float nearestStart[3] = {};
		float nearestEnd[3] = {};
		if (!TryFindNearestPolyPoint(query, startPos, extents, filter, startRef, nearestStart) ||
			!TryFindNearestPolyPoint(query, endPos, extents, filter, endRef, nearestEnd))
		{
			return false;
		}

		static constexpr int kMaxPathPolys = 32;
		dtPolyRef path[kMaxPathPolys] = {};
		int pathCount = 0;
		const dtStatus pathStatus = query->findPath(
			startRef,
			endRef,
			nearestStart,
			nearestEnd,
			&filter,
			path,
			&pathCount,
			kMaxPathPolys);
		if (dtStatusFailed(pathStatus) || pathCount <= 0)
			return false;

		static constexpr int kMaxStraightPath = 8;
		float straightPath[kMaxStraightPath * 3] = {};
		unsigned char straightPathFlags[kMaxStraightPath] = {};
		dtPolyRef straightPathRefs[kMaxStraightPath] = {};
		int straightPathCount = 0;
		const dtStatus straightStatus = query->findStraightPath(
			nearestStart,
			nearestEnd,
			path,
			pathCount,
			straightPath,
			straightPathFlags,
			straightPathRefs,
			&straightPathCount,
			kMaxStraightPath);
		if (dtStatusFailed(straightStatus) || straightPathCount <= 0)
			return false;

		const int cornerIndex = (straightPathCount > 1) ? 1 : 0;
		outCorner =
		{
			straightPath[cornerIndex * 3 + 0],
			straightPath[cornerIndex * 3 + 1],
			straightPath[cornerIndex * 3 + 2]
		};
		return true;
	}
}

void AIMovementPolicyUtil::ClearMove(AIContext& ctx)
{
	if (ctx.intent == nullptr)
		return;

	ctx.intent->hasMove = false;
	ctx.intent->wantsRun = false;
	ctx.intent->moveDir = { 0.0f, 0.0f, 0.0f };
}

bool AIMovementPolicyUtil::StoreMoveDirection(
	AIContext& ctx,
	DirectX::XMFLOAT3 moveDir,
	bool wantsRun)
{
	if (ctx.intent == nullptr)
		return false;

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

	ctx.intent->hasMove = true;
	ctx.intent->wantsRun = wantsRun;
	ctx.intent->moveDir = moveDir;
	return true;
}

bool AIMovementPolicyUtil::TryGetCurrentTargetPosition(
	AIContext& ctx,
	DirectX::XMFLOAT3& outTargetPos)
{
	if (ctx.blackboard == nullptr ||
		ctx.blackboard->currentTarget.IsNull() ||
		ctx.sysCtx == nullptr)
	{
		return false;
	}

	const WorldTransformComp* targetTr =
		ctx.sysCtx->ecs.GetComponent<WorldTransformComp>(
			ctx.blackboard->currentTarget);
	if (targetTr == nullptr)
		return false;

	outTargetPos = targetTr->position;
	return true;
}

void AIMovementPolicyUtil::BuildDestinationIntent(
	AIContext& ctx,
	const DirectX::XMFLOAT3& destination,
	bool wantsRun)
{
	if (ctx.intent  == nullptr ||
		ctx.selfTr == nullptr ||
		ctx.movementRuntime == nullptr)
	{
		return;
	}

	ctx.movementRuntime->pathRecomputeAcc +=
		ctx.sysCtx ? ctx.sysCtx->dtSec : kPathRecomputeIntervalSec;

	const bool destinationChanged =
		DistanceSqXZ(ctx.movementRuntime->pathDestination, destination) >
		kDestinationChangeEpsilonSq;
	const bool cornerReached =
		ctx.movementRuntime->hasPathCorner &&
		DistanceSqXZ(ctx.selfTr->position, ctx.movementRuntime->nextPathCorner) <=
		kPathCornerArriveRangeSq;
	const bool shouldRecompute =
		!ctx.movementRuntime->hasPathCorner ||
		destinationChanged ||
		cornerReached ||
		ctx.movementRuntime->pathRecomputeAcc >= kPathRecomputeIntervalSec;

	if (shouldRecompute)
	{
		ctx.movementRuntime->pathDestination = destination;
		ctx.movementRuntime->pathRecomputeAcc = 0.0;

		DirectX::XMFLOAT3 corner{};
		if (TryBuildPathCorner(ctx, destination, corner))
		{
			ctx.movementRuntime->nextPathCorner = corner;
			ctx.movementRuntime->hasPathCorner = true;
		}
		else
		{
			ctx.movementRuntime->nextPathCorner = destination;
			ctx.movementRuntime->hasPathCorner = false;
		}
	}

	const DirectX::XMFLOAT3 moveTarget =
		ctx.movementRuntime->hasPathCorner
		? ctx.movementRuntime->nextPathCorner
		: destination;
	TryStoreDirectionTo(ctx, moveTarget, wantsRun);
}
