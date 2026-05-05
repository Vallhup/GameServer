#include "pch.h"
#include "NormalAIMovementPolicy.h"

#include "AIBehaviorDef.h"
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
	static constexpr float kMoveEpsilonSq = 1.0e-6f;
	static constexpr double kPathRecomputeIntervalSec = 0.35;
	static constexpr double kDestinationChangeEpsilonSq = 0.25;
	static constexpr double kPathCornerArriveRangeSq = 0.25;

	static dtQueryFilter BuildQueryFilter(const NavigationProfileDef* profile)
	{
		dtQueryFilter filter;
		if (profile)
		{
			filter.setIncludeFlags(profile->queryFilter.includeFlags);
			filter.setExcludeFlags(profile->queryFilter.excludeFlags);
			filter.setAreaCost(RC_WALKABLE_AREA, profile->queryFilter.walkableAreaCost);
		}
		return filter;
	}

	static bool TryFindNearestPolyPoint(
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

	static double DistanceSqXZ(
		const XMFLOAT3& a,
		const XMFLOAT3& b) noexcept
	{
		const double dx = static_cast<double>(a.x) - static_cast<double>(b.x);
		const double dz = static_cast<double>(a.z) - static_cast<double>(b.z);
		return dx * dx + dz * dz;
	}

	static bool TryStoreDirectionTo(
		AIContext& ctx,
		const XMFLOAT3& destination,
		bool wantsRun)
	{
		XMVECTOR dir = TransformHelper::Direction(ctx.selfTr->position, destination);
		const float dirLengthSq = XMVectorGetX(XMVector3LengthSq(dir));
		if (dirLengthSq <= kMoveEpsilonSq)
		{
			ctx.command->hasMove = false;
			ctx.command->wantsRun = false;
			ctx.command->moveDir = { 0.0f, 0.0f, 0.0f };
			return false;
		}

		XMStoreFloat3(&ctx.command->moveDir, dir);
		ctx.command->hasMove = true;
		ctx.command->wantsRun = wantsRun;
		return true;
	}

	static bool TryBuildPathCorner(
		AIContext& ctx,
		const XMFLOAT3& destination,
		XMFLOAT3& outCorner)
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
		if (!TryFindNearestPolyPoint(
			query,
			startPos,
			extents,
			filter,
			startRef,
			nearestStart) ||
			!TryFindNearestPolyPoint(
				query,
				endPos,
				extents,
				filter,
				endRef,
				nearestEnd))
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

void NormalAIMovementPolicy::BuildChaseIntent(AIContext& ctx) const
{
	XMFLOAT3 targetPos;
	if (!TryGetCurrentTargetPosition(ctx, targetPos))
		return;

	BuildDestinationIntent(ctx, targetPos, true);
}

void NormalAIMovementPolicy::BuildCombatIntent(AIContext& ctx) const
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

void NormalAIMovementPolicy::BuildSearchIntent(AIContext& ctx) const
{
	if (!ctx.blackboard->hasLastKnownTargetPosition) return;

	BuildDestinationIntent(ctx, ctx.blackboard->lastKnownTargetPosition, false);
}

void NormalAIMovementPolicy::BuildReturnHomeIntent(AIContext& ctx) const
{
	if (ctx.blackboard == nullptr || !ctx.blackboard->hasHomePosition)
		return;

	BuildDestinationIntent(ctx, ctx.blackboard->homePosition, false);
}

void NormalAIMovementPolicy::BuildDestinationIntent(
	AIContext& ctx,
	const XMFLOAT3& destination,
	bool wantsRun) const
{
	if (ctx.command == nullptr ||
		ctx.selfTr == nullptr ||
		ctx.blackboard == nullptr)
	{
		return;
	}

	ctx.blackboard->pathRecomputeAcc +=
		ctx.sysCtx ? ctx.sysCtx->dtSec : kPathRecomputeIntervalSec;

	const bool destinationChanged =
		DistanceSqXZ(ctx.blackboard->pathDestination, destination) >
		kDestinationChangeEpsilonSq;
	const bool cornerReached =
		ctx.blackboard->hasPathCorner &&
		DistanceSqXZ(ctx.selfTr->position, ctx.blackboard->nextPathCorner) <=
		kPathCornerArriveRangeSq;
	const bool shouldRecompute =
		!ctx.blackboard->hasPathCorner ||
		destinationChanged ||
		cornerReached ||
		ctx.blackboard->pathRecomputeAcc >= kPathRecomputeIntervalSec;

	if (shouldRecompute)
	{
		ctx.blackboard->pathDestination = destination;
		ctx.blackboard->pathRecomputeAcc = 0.0;

		XMFLOAT3 corner{};
		if (TryBuildPathCorner(ctx, destination, corner))
		{
			ctx.blackboard->nextPathCorner = corner;
			ctx.blackboard->hasPathCorner = true;
		}
		else
		{
			ctx.blackboard->nextPathCorner = destination;
			ctx.blackboard->hasPathCorner = false;
		}
	}

	const XMFLOAT3 moveTarget =
		ctx.blackboard->hasPathCorner ? ctx.blackboard->nextPathCorner : destination;
	TryStoreDirectionTo(ctx, moveTarget, wantsRun);
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
