#include "pch.h"
#include "ResolveNavMeshBodyConstraintSystem.h"

#include "../../../Library/Recast/Recast.h"
#include "../../../Library/Detour/DetourNavMesh.h"
#include "../../../Library/Detour/DetourNavMeshQuery.h"

#include "../../GameplayRuntimeComponents.h"
#include "../GameplaySystemUtil.h"

#include "BodyCollisionSlide.h"
#include "NavMeshRuntime.h"
#include "WorldDef.h"

using namespace GameplaySystemUtil;

static const std::array<AccessSpec, 8> kResolveNavMeshBodyConstraintAccesses{
	ReadImmediate(ExternalRes<INavMeshProvider>()),
	ReadImmediate(ExternalRes<ITerrainHeightProvider>()),
	WriteImmediate(ComponentRes<WorldTransformComp>()),
	ReadImmediate(ComponentRes<PreCollisionTransformComp>()),
	ReadImmediate(ComponentRes<BodyCollisionShapeComp>()),
	WriteImmediate(ComponentRes<NavMeshAgentStateComp>()),
	WriteImmediate(ComponentRes<BodyCollisionResolveComp>()),
	WriteImmediate(ComponentRes<DirtyFlagsComp>()),
};

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

static bool TryResolveStartPoly(
	dtNavMeshQuery* query,
	const dtQueryFilter& filter,
	const float* extents,
	const float* prevPos,
	const float* candidatePos,
	dtPolyRef cachedRef,
	dtPolyRef& outRef,
	float* outStartPos)
{
	outRef = 0;

	if (cachedRef != 0 && query->isValidPolyRef(cachedRef, &filter))
	{
		bool prevPosOverPoly = false;
		float projectedPrevPos[3] = {};
		const dtStatus closestStatus = query->closestPointOnPoly(
			cachedRef,
			prevPos,
			projectedPrevPos,
			&prevPosOverPoly);

		if (!dtStatusFailed(closestStatus) && prevPosOverPoly)
		{
			outRef = cachedRef;
			outStartPos[0] = projectedPrevPos[0];
			outStartPos[1] = projectedPrevPos[1];
			outStartPos[2] = projectedPrevPos[2];
			return true;
		}
	}

	if (TryFindNearestPolyPoint(
		query,
		prevPos,
		extents,
		filter,
		outRef,
		outStartPos))
	{
		return true;
	}

	return TryFindNearestPolyPoint(
		query,
		candidatePos,
		extents,
		filter,
		outRef,
		outStartPos);
}

const StaticSystemMetaStorage<8> ResolveNavMeshBodyConstraintSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<ResolveNavMeshBodyConstraintSystem>(),
		"ResolveNavMeshBodyConstraintSystem",
		std::array<AccessSpec, 8>
	{
		ReadImmediate(ExternalRes<INavMeshProvider>()),
		ReadImmediate(ExternalRes<ITerrainHeightProvider>()),
		WriteImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<PreCollisionTransformComp>()),
		ReadImmediate(ComponentRes<BodyCollisionShapeComp>()),
		WriteImmediate(ComponentRes<NavMeshAgentStateComp>()),
		WriteImmediate(ComponentRes<BodyCollisionResolveComp>()),
		WriteImmediate(ComponentRes<DirtyFlagsComp>()),
	});

void ResolveNavMeshBodyConstraintSystem::Execute(SystemContext& ctx)
{
	const INavMeshProvider* navProvider = ctx.services.navMeshProvider;
	const ITerrainHeightProvider* terrainProvider = ctx.services.terrainHeightProvider;
	const NavMeshRuntime* navMesh =
		navProvider ? navProvider->GetNavMeshRuntime() : nullptr;
	const NavigationProfileDef* profile =
		navProvider ? navProvider->GetNavigationProfile() : nullptr;

	const bool hasValidNavMesh = (navMesh != nullptr && navMesh->IsReady());

	for (auto [entity,
		transform,
		preCollision,
		bodyShape,
		navAgent,
		resolveState] :
		ctx.ecs.View<
			WorldTransformComp,
			PreCollisionTransformComp,
			BodyCollisionShapeComp,
			NavMeshAgentStateComp,
			BodyCollisionResolveComp>())
	{
		const XMFLOAT3 collisionSlideDelta =
			resolveState.collisionSlideDelta;
		const bool overlapAdjusted = resolveState.overlapAdjusted;
		const bool slideAdjusted = resolveState.slideAdjusted;
		resolveState = {};
		resolveState.collisionSlideDelta = collisionSlideDelta;
		resolveState.overlapAdjusted = overlapAdjusted;
		resolveState.slideAdjusted = slideAdjusted;
		resolveState.navResolvedPosition = transform.position;

		auto MarkTransformDirtyIfPresent =
			[&ctx, entity]()
			{
				if (DirtyFlagsComp* dirty =
					ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity))
				{
					dirty->MarkDirty(WorldDirtyType::Transform);
				}
			};

		auto RejectToPreviousPosition =
			[&]()
			{
				resolveState.rejectedByNavMesh = true;
				transform.position = preCollision.prevPosition;
				resolveState.navResolvedPosition = transform.position;
				navAgent.currentPolyRef = 0;
				MarkTransformDirtyIfPresent();
			};

		if (!bodyShape.useNavMeshConstraint)
			continue;

		if (!hasValidNavMesh)
		{
			resolveState.navMeshFallbackNoProvider = true;
			resolveState.navResolvedPosition = transform.position;
			continue;
		}

		const bool needsInitialProjection = navAgent.currentPolyRef == 0;
		if (!preCollision.movedThisFrame &&
			!preCollision.preserveAbilityVerticalAboveNavMesh &&
			!needsInitialProjection)
		{
			resolveState.navResolvedPosition = transform.position;
			continue;
		}

		dtNavMeshQuery* query = navMesh->GetQuery();
		dtQueryFilter filter = BuildQueryFilter(profile);

		const float extentXZ = profile ? profile->nearestPolyExtentXZ : 2.0f;
		const float extentY = profile ? profile->nearestPolyExtentY : 4.0f;
		const float surfaceYOffset = profile ? profile->navMeshSurfaceYOffset : 0.0f;
		const float extents[3] = { extentXZ, extentY, extentXZ };

		const float prevPos[3] = 
		{
			preCollision.prevPosition.x,
			preCollision.prevPosition.y,
			preCollision.prevPosition.z,
		};
		const float candidatePos[3] = 
		{
			transform.position.x,
			transform.position.y,
			transform.position.z,
		};

		dtPolyRef startRef = 0;
		float startPos[3] = {};
		if (!TryResolveStartPoly(
			query,
			filter,
			extents,
			prevPos,
			candidatePos,
			static_cast<dtPolyRef>(navAgent.currentPolyRef),
			startRef,
			startPos))
		{
			RejectToPreviousPosition();
			continue;
		}

		float resultPos[3] = {};
		static constexpr int kMaxVisited = 16;
		dtPolyRef visited[kMaxVisited] = {};
		int visitedCount = 0;

		const dtStatus moveStatus = query->moveAlongSurface(
			startRef,
			startPos,
			candidatePos,
			&filter,
			resultPos,
			visited,
			&visitedCount,
			kMaxVisited);

		if (dtStatusFailed(moveStatus))
		{
			RejectToPreviousPosition();
			continue;
		}

		const float correctionX = resultPos[0] - candidatePos[0];
		const float correctionZ = resultPos[2] - candidatePos[2];
		if (LengthXZ(correctionX, correctionZ) > kOverlapEpsilon)
		{
			const BodyCollisionSlide::XZDelta slideAdjustment =
				BodyCollisionSlide::ComputeTangentPreservingAdjustment(
					candidatePos[0] - startPos[0],
					candidatePos[2] - startPos[2],
					resultPos[0] - startPos[0],
					resultPos[2] - startPos[2],
					correctionX,
					correctionZ,
					kOverlapEpsilon);

			if (LengthXZ(slideAdjustment.x, slideAdjustment.z) >
				kOverlapEpsilon)
			{
				const float slideTarget[3] =
				{
					resultPos[0] + slideAdjustment.x,
					resultPos[1],
					resultPos[2] + slideAdjustment.z,
				};
				float slideResult[3] = {};
				dtPolyRef slideVisited[kMaxVisited] = {};
				int slideVisitedCount = 0;
				const dtStatus slideStatus = query->moveAlongSurface(
					startRef,
					startPos,
					slideTarget,
					&filter,
					slideResult,
					slideVisited,
					&slideVisitedCount,
					kMaxVisited);

				if (!dtStatusFailed(slideStatus))
				{
					const float appliedSlideX = slideResult[0] - resultPos[0];
					const float appliedSlideZ = slideResult[2] - resultPos[2];
					resultPos[0] = slideResult[0];
					resultPos[1] = slideResult[1];
					resultPos[2] = slideResult[2];
					std::copy_n(
						slideVisited,
						slideVisitedCount,
						visited);
					visitedCount = slideVisitedCount;

					if (LengthXZ(appliedSlideX, appliedSlideZ) >
						kOverlapEpsilon)
					{
						resolveState.collisionSlideDelta.x += appliedSlideX;
						resolveState.collisionSlideDelta.z += appliedSlideZ;
						resolveState.slideAdjusted = true;
					}
				}
			}
		}

		dtPolyRef resultRef =
			(visitedCount > 0) ? visited[visitedCount - 1] : startRef;

		float surfaceHeight = resultPos[1];
		dtStatus heightStatus =
			query->getPolyHeight(resultRef, resultPos, &surfaceHeight);
		if (dtStatusFailed(heightStatus))
		{
			float nearestPt[3] = {};
			dtPolyRef nearestRef = 0;
			if (!TryFindNearestPolyPoint(
				query,
				resultPos,
				extents,
				filter,
				nearestRef,
				nearestPt))
			{
				RejectToPreviousPosition();
				continue;
			}

			resultRef = nearestRef;
			resultPos[0] = nearestPt[0];
			resultPos[1] = nearestPt[1];
			resultPos[2] = nearestPt[2];
			surfaceHeight = resultPos[1];
			heightStatus = query->getPolyHeight(resultRef, resultPos, &surfaceHeight);
			if (dtStatusFailed(heightStatus))
			{
				RejectToPreviousPosition();
				continue;
			}
		}
		float floorY = surfaceHeight + surfaceYOffset;
		if (terrainProvider)
		{
			float terrainY = 0.0f;
			if (terrainProvider->TrySampleHeight(resultPos[0], resultPos[2], terrainY))
			{
				floorY = terrainY;
				resolveState.terrainHeightAdjusted = true;
			}
			else
			{
				resolveState.terrainHeightSampleFailed = true;
			}
		}
		else
		{
			resolveState.terrainHeightFallbackNoProvider = true;
		}

		resultPos[1] = preCollision.preserveAbilityVerticalAboveNavMesh
			? std::max(transform.position.y, floorY)
			: floorY;

		navAgent.currentPolyRef = static_cast<uint64_t>(resultRef);

		const XMFLOAT3 resolvedPosition =
		{
			resultPos[0], resultPos[1], resultPos[2],
		};
		const bool positionChanged =
			std::abs(transform.position.x - resolvedPosition.x) > kOverlapEpsilon ||
			std::abs(transform.position.y - resolvedPosition.y) > kOverlapEpsilon ||
			std::abs(transform.position.z - resolvedPosition.z) > kOverlapEpsilon;

		transform.position = resolvedPosition;
		resolveState.navMeshAdjusted = positionChanged;
		resolveState.navResolvedPosition = transform.position;

		if (positionChanged)
		{
			MarkTransformDirtyIfPresent();
		}
	}
}
