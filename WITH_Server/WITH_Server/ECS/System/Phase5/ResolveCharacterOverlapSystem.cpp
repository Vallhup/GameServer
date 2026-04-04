#include "pch.h"
#include "ResolveCharacterOverlapSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta ResolveCharacterOverlapSystem::kMeta =
	MakeSystemMeta<ResolveCharacterOverlapSystem>(
		"ResolveCharacterOverlapSystem");

void ResolveCharacterOverlapSystem::Execute(SystemContext& ctx)
{
	std::vector<Entity> entities;
	for (auto [entity, transform, shape] :
		ctx.ecs.View<WorldTransformComp, BodyCollisionShapeComp>())
	{
		(void)transform;
		if (shape.blocksBodyOverlap &&
			shape.pushability != BodyPushability::None)
		{
			entities.push_back(entity);
		}
	}

	std::sort(
		entities.begin(),
		entities.end(),
		[](Entity lhs, Entity rhs)
		{
			return lhs.id < rhs.id;
		});

	for (size_t i = 0; i < entities.size(); ++i)
	{
		for (size_t j = i + 1; j < entities.size(); ++j)
		{
			auto* lhsTransform =
				MutableComponent<WorldTransformComp>(ctx.ecs, entities[i]);
			auto* rhsTransform =
				MutableComponent<WorldTransformComp>(ctx.ecs, entities[j]);
			auto* lhsShape =
				MutableComponent<BodyCollisionShapeComp>(ctx.ecs, entities[i]);
			auto* rhsShape =
				MutableComponent<BodyCollisionShapeComp>(ctx.ecs, entities[j]);
			if (lhsTransform == nullptr || rhsTransform == nullptr ||
				lhsShape == nullptr || rhsShape == nullptr)
			{
				continue;
			}

			const float dx =
				rhsTransform->position.x - lhsTransform->position.x;
			const float dz =
				rhsTransform->position.z - lhsTransform->position.z;
			const float distance = LengthXZ(dx, dz);
			const float minDistance =
				lhsShape->bodyRadiusXZ + rhsShape->bodyRadiusXZ;
			const float penetration = minDistance - distance;
			if (penetration <= 0.0f)
			{
				continue;
			}

			const float nx =
				(distance > kOverlapEpsilon) ? dx / distance : 1.0f;
			const float nz =
				(distance > kOverlapEpsilon) ? dz / distance : 0.0f;
			const float halfPush = 0.5f * penetration;

			if (lhsShape->pushability == BodyPushability::Dynamic)
			{
				lhsTransform->position.x -= nx * halfPush;
				lhsTransform->position.z -= nz * halfPush;
			}
			if (rhsShape->pushability == BodyPushability::Dynamic)
			{
				rhsTransform->position.x += nx * halfPush;
				rhsTransform->position.z += nz * halfPush;
			}

			if (BodyCollisionResolveComp* resolve =
				MutableComponent<BodyCollisionResolveComp>(
					ctx.ecs,
					entities[i]))
			{
				resolve->overlapAdjusted = true;
			}
			if (BodyCollisionResolveComp* resolve =
				MutableComponent<BodyCollisionResolveComp>(
					ctx.ecs,
					entities[j]))
			{
				resolve->overlapAdjusted = true;
			}
		}
	}
}

const SystemMeta& ResolveCharacterOverlapSystem::Meta() const
{
	return kMeta;
}
