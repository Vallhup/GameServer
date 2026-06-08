#include "pch.h"
#include "ResolveGimmickObjectOverlapSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	inline constexpr float kHeightEpsilon = 0.1f;

	struct PlayerEntry
	{
		Entity entity{ Entity::Null() };
		WorldTransformComp* transform{ nullptr };
		PreCollisionTransformComp* preCollision{ nullptr };
		BodyCollisionShapeComp* shape{ nullptr };
		BodyCollisionResolveComp* resolve{ nullptr };
	};

	struct ObjectEntry
	{
		Entity entity{ Entity::Null() };
		const WorldTransformComp* transform{ nullptr };
		const StaticBoxHurtColliderComp* box{ nullptr };
	};

	bool HasVerticalOverlap(
		const WorldTransformComp& playerTransform,
		const BodyCollisionShapeComp& playerShape,
		const WorldTransformComp& objectTransform,
		const StaticBoxHurtColliderComp& objectBox) noexcept
	{
		const float playerMinY = playerTransform.position.y;
		const float playerMaxY =
			playerTransform.position.y + playerShape.bodyHeight;
		const float objectMinY =
			objectTransform.position.y - objectBox.halfExtents.y;
		const float objectMaxY =
			objectTransform.position.y + objectBox.halfExtents.y;

		return !(playerMaxY < objectMinY - kHeightEpsilon ||
			objectMaxY < playerMinY - kHeightEpsilon);
	}

	void ResolveInsideBoxNormal(
		const PlayerEntry& player,
		const ObjectEntry& object,
		float minX,
		float maxX,
		float minZ,
		float maxZ,
		float& outNx,
		float& outNz,
		float& outDistanceToFace) noexcept
	{
		const float left = player.transform->position.x - minX;
		const float right = maxX - player.transform->position.x;
		const float back = player.transform->position.z - minZ;
		const float front = maxZ - player.transform->position.z;

		outNx = -1.0f;
		outNz = 0.0f;
		outDistanceToFace = left;

		if (right < outDistanceToFace)
		{
			outNx = 1.0f;
			outNz = 0.0f;
			outDistanceToFace = right;
		}
		if (back < outDistanceToFace)
		{
			outNx = 0.0f;
			outNz = -1.0f;
			outDistanceToFace = back;
		}
		if (front < outDistanceToFace)
		{
			outNx = 0.0f;
			outNz = 1.0f;
			outDistanceToFace = front;
		}

		if (outDistanceToFace > kOverlapEpsilon)
		{
			return;
		}

		float dirX =
			player.transform->position.x - player.preCollision->prevPosition.x;
		float dirZ =
			player.transform->position.z - player.preCollision->prevPosition.z;
		if (!TransformHelper::NormalizeXZ(dirX, dirZ))
		{
			dirX = player.transform->position.x - object.transform->position.x;
			dirZ = player.transform->position.z - object.transform->position.z;
			if (!TransformHelper::NormalizeXZ(dirX, dirZ))
			{
				dirX = ((player.entity.id ^ object.entity.id) & 1u) == 0u
					? 1.0f
					: 0.0f;
				dirZ = dirX == 0.0f ? 1.0f : 0.0f;
			}
		}

		outNx = dirX;
		outNz = dirZ;
		outDistanceToFace = 0.0f;
	}

	bool TryBuildCircleBoxCorrection(
		const PlayerEntry& player,
		const ObjectEntry& object,
		float& outCorrectionX,
		float& outCorrectionZ) noexcept
	{
		const float radius = std::max(0.0f, player.shape->bodyRadiusXZ);
		if (radius <= kOverlapEpsilon)
		{
			return false;
		}

		const float minX =
			object.transform->position.x - object.box->halfExtents.x;
		const float maxX =
			object.transform->position.x + object.box->halfExtents.x;
		const float minZ =
			object.transform->position.z - object.box->halfExtents.z;
		const float maxZ =
			object.transform->position.z + object.box->halfExtents.z;

		const float playerX = player.transform->position.x;
		const float playerZ = player.transform->position.z;
		const bool insideX = playerX >= minX && playerX <= maxX;
		const bool insideZ = playerZ >= minZ && playerZ <= maxZ;

		float nx = 0.0f;
		float nz = 0.0f;
		float penetration = 0.0f;

		if (insideX && insideZ)
		{
			float distanceToFace = 0.0f;
			ResolveInsideBoxNormal(
				player,
				object,
				minX,
				maxX,
				minZ,
				maxZ,
				nx,
				nz,
				distanceToFace);
			penetration = radius + distanceToFace;
		}
		else
		{
			const float closestX = std::clamp(playerX, minX, maxX);
			const float closestZ = std::clamp(playerZ, minZ, maxZ);
			nx = playerX - closestX;
			nz = playerZ - closestZ;

			const float distance = LengthXZ(nx, nz);
			if (distance >= radius)
			{
				return false;
			}
			if (distance <= kOverlapEpsilon)
			{
				nx = playerX - object.transform->position.x;
				nz = playerZ - object.transform->position.z;
				if (!TransformHelper::NormalizeXZ(nx, nz))
				{
					nx = 1.0f;
					nz = 0.0f;
				}
				penetration = radius;
			}
			else
			{
				nx /= distance;
				nz /= distance;
				penetration = radius - distance;
			}
		}

		if (penetration <= kOverlapEpsilon)
		{
			return false;
		}

		outCorrectionX = nx * penetration;
		outCorrectionZ = nz * penetration;
		return true;
	}
}

const StaticSystemMetaStorage<10> ResolveGimmickObjectOverlapSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<ResolveGimmickObjectOverlapSystem>(),
		"ResolveGimmickObjectOverlapSystem",
		std::array<AccessSpec, 10>
	{
		WriteImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<PreCollisionTransformComp>()),
		ReadImmediate(ComponentRes<BodyCollisionShapeComp>()),
		WriteImmediate(ComponentRes<BodyCollisionResolveComp>()),
		WriteImmediate(ComponentRes<DirtyFlagsComp>()),
		ReadImmediate(ComponentRes<PlayerControlIdentityComp>()),
		ReadImmediate(ComponentRes<StaticBoxHurtColliderComp>()),
		ReadImmediate(ComponentRes<GimmickObjectComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
	});

void ResolveGimmickObjectOverlapSystem::Execute(SystemContext& ctx)
{
	std::vector<PlayerEntry> players;
	for (auto [entity, player, transform, preCollision, shape, resolve] :
		ctx.ecs.View<
			PlayerControlIdentityComp,
			WorldTransformComp,
			PreCollisionTransformComp,
			BodyCollisionShapeComp,
			BodyCollisionResolveComp>())
	{
		if (player.ownerSessionId != 0 &&
			shape.blocksBodyOverlap &&
			shape.pushability != BodyPushability::None &&
			!HasBlockingPendingState(ctx.ecs, entity))
		{
			players.push_back(PlayerEntry{
				entity,
				&transform,
				&preCollision,
				&shape,
				&resolve
			});
		}
	}

	std::vector<ObjectEntry> objects;
	for (auto [entity, transform, box, gimmick] :
		ctx.ecs.View<
			WorldTransformComp,
			StaticBoxHurtColliderComp,
			GimmickObjectComp>())
	{
		if (!gimmick.broken && !HasBlockingPendingState(ctx.ecs, entity))
		{
			objects.push_back(ObjectEntry{
				entity,
				&transform,
				&box
			});
		}
	}

	std::sort(players.begin(), players.end(), [](const auto& lhs, const auto& rhs) {
		return lhs.entity.id < rhs.entity.id;
	});
	std::sort(objects.begin(), objects.end(), [](const auto& lhs, const auto& rhs) {
		return lhs.entity.id < rhs.entity.id;
	});

	for (PlayerEntry& player : players)
	{
		float totalCorrectionX = 0.0f;
		float totalCorrectionZ = 0.0f;

		for (const ObjectEntry& object : objects)
		{
			if (!HasVerticalOverlap(
					*player.transform,
					*player.shape,
					*object.transform,
					*object.box))
			{
				continue;
			}

			float correctionX = 0.0f;
			float correctionZ = 0.0f;
			if (!TryBuildCircleBoxCorrection(
					player,
					object,
					correctionX,
					correctionZ))
			{
				continue;
			}

			totalCorrectionX += correctionX;
			totalCorrectionZ += correctionZ;
		}

		const float correctionLength =
			LengthXZ(totalCorrectionX, totalCorrectionZ);
		if (correctionLength <= kOverlapEpsilon)
		{
			continue;
		}

		const float maxCorrection =
			std::max(player.shape->maxOverlapCorrectionPerFrameXZ, 0.0f);
		if (maxCorrection > kOverlapEpsilon &&
			correctionLength > maxCorrection)
		{
			const float scale = maxCorrection / correctionLength;
			totalCorrectionX *= scale;
			totalCorrectionZ *= scale;
		}
		else if (maxCorrection <= kOverlapEpsilon)
		{
			continue;
		}

		player.transform->position.x += totalCorrectionX;
		player.transform->position.z += totalCorrectionZ;
		player.resolve->overlapAdjusted = true;

		if (DirtyFlagsComp* dirty =
			ctx.ecs.GetMutableComponent<DirtyFlagsComp>(player.entity))
		{
			dirty->MarkDirty(WorldDirtyType::Transform);
		}
	}
}
