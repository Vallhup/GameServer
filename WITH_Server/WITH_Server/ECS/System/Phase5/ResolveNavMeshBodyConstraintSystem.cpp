#include "pch.h"
#include "ResolveNavMeshBodyConstraintSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta ResolveNavMeshBodyConstraintSystem::kMeta =
	MakeSystemMeta<ResolveNavMeshBodyConstraintSystem>(
		"ResolveNavMeshBodyConstraintSystem");

void ResolveNavMeshBodyConstraintSystem::Execute(SystemContext& ctx)
{
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
		(void)entity;
		(void)navAgent;
		resolveState = {};
		resolveState.navResolvedPosition = transform.position;

		if (!bodyShape.useNavMeshConstraint)
		{
			continue;
		}

		// v0.1 fallback: Recast/Detour query provider is not wired yet.
		resolveState.navMeshFallbackNoProvider = true;
		resolveState.navResolvedPosition = preCollision.candidatePosition;
	}
}

const SystemMeta& ResolveNavMeshBodyConstraintSystem::Meta() const
{
	return kMeta;
}
