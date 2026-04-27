#include "pch.h"
#include "ResolveCharacterOverlapSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	inline constexpr float kHeightEpsilon = 0.1f;

	const std::array<AccessSpec, 7> kResolveCharacterOverlapAccesses{
		WriteImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<PreCollisionTransformComp>()),
		ReadImmediate(ComponentRes<BodyCollisionShapeComp>()),
		WriteImmediate(ComponentRes<BodyCollisionResolveComp>()),
		WriteImmediate(ComponentRes<DirtyFlagsComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
	};

	struct OverlapEntry
	{
		Entity entity{ Entity::Null() };
		WorldTransformComp* transform{ nullptr };
		PreCollisionTransformComp* preCollision{ nullptr };
		BodyCollisionShapeComp* shape{ nullptr };
		BodyCollisionResolveComp* resolve{ nullptr };
	};

	struct XZCorrection
	{
		float x{ 0.0f };
		float z{ 0.0f };
	};

	bool HasVerticalOverlap(
		const WorldTransformComp& lhsTransform,
		const BodyCollisionShapeComp& lhsShape,
		const WorldTransformComp& rhsTransform,
		const BodyCollisionShapeComp& rhsShape) noexcept
	{
		const float lhsMinY = lhsTransform.position.y;
		const float lhsMaxY = lhsTransform.position.y + lhsShape.bodyHeight;
		const float rhsMinY = rhsTransform.position.y;
		const float rhsMaxY = rhsTransform.position.y + rhsShape.bodyHeight;

		return !(lhsMaxY < rhsMinY - kHeightEpsilon ||
			rhsMaxY < lhsMinY - kHeightEpsilon);
	}

	void ResolveZeroDistanceNormal(
		const OverlapEntry& lhs,
		const OverlapEntry& rhs,
		bool lhsMoved,
		bool rhsMoved,
		float& outNx,
		float& outNz) noexcept
	{
		float dirX = 0.0f;
		float dirZ = 0.0f;

		if (lhsMoved != rhsMoved)
		{
			if (lhsMoved)
			{
				dirX = lhs.transform->position.x - lhs.preCollision->prevPosition.x;
				dirZ = lhs.transform->position.z - lhs.preCollision->prevPosition.z;
			}
			else
			{
				dirX = rhs.preCollision->prevPosition.x - rhs.transform->position.x;
				dirZ = rhs.preCollision->prevPosition.z - rhs.transform->position.z;
			}
		}

		if (LengthXZ(dirX, dirZ) <= kOverlapEpsilon)
		{
			outNx = ((lhs.entity.id ^ rhs.entity.id) & 1u) == 0u ? 1.0f : 0.0f;
			outNz = (outNx == 0.0f) ? 1.0f : 0.0f;
			return;
		}

		NormalizeXZ(dirX, dirZ);
		outNx = dirX;
		outNz = dirZ;
	}
}

const SystemMeta ResolveCharacterOverlapSystem::kMeta =
	SystemMeta{
		SysTag<ResolveCharacterOverlapSystem>(),
		"ResolveCharacterOverlapSystem",
		kResolveCharacterOverlapAccesses,
		kNoDeps,
		kNoDeps
	};

void ResolveCharacterOverlapSystem::Execute(SystemContext& ctx)
{
	std::vector<OverlapEntry> entries;
	for (auto [entity, transform, preCollision, shape, resolve] :
		ctx.ecs.View<
			WorldTransformComp,
			PreCollisionTransformComp,
			BodyCollisionShapeComp,
			BodyCollisionResolveComp>())
	{
		resolve.overlapAdjusted = false;

		if (shape.blocksBodyOverlap &&
			shape.pushability != BodyPushability::None &&
			!HasBlockingPendingState(ctx.ecs, entity))
		{
			entries.push_back(
				OverlapEntry{
					entity,
					&transform,
					&preCollision,
					&shape,
					&resolve
				});
		}
	}

	std::sort(
		entries.begin(),
		entries.end(),
		[](const OverlapEntry& lhs, const OverlapEntry& rhs)
		{
			return lhs.entity.id < rhs.entity.id;
		});

	std::vector<XZCorrection> accumulated(entries.size());

	for (size_t i = 0; i < entries.size(); ++i)
	{
		for (size_t j = i + 1; j < entries.size(); ++j)
		{
			OverlapEntry& lhs = entries[i];
			OverlapEntry& rhs = entries[j];
			if (lhs.transform == nullptr || rhs.transform == nullptr ||
				lhs.preCollision == nullptr || rhs.preCollision == nullptr ||
				lhs.shape == nullptr || rhs.shape == nullptr)
			{
				continue;
			}

			if (!HasVerticalOverlap(
				*lhs.transform,
				*lhs.shape,
				*rhs.transform,
				*rhs.shape))
			{
				continue;
			}

			const float lhsX = lhs.transform->position.x + accumulated[i].x;
			const float lhsZ = lhs.transform->position.z + accumulated[i].z;
			const float rhsX = rhs.transform->position.x + accumulated[j].x;
			const float rhsZ = rhs.transform->position.z + accumulated[j].z;

			const float dx = rhsX - lhsX;
			const float dz = rhsZ - lhsZ;
			const float distance = LengthXZ(dx, dz);
			const float minDistance =
				lhs.shape->bodyRadiusXZ + rhs.shape->bodyRadiusXZ;
			const float penetration = minDistance - distance;
			if (penetration <= 0.0f)
			{
				continue;
			}

			float nx = 0.0f;
			float nz = 0.0f;
			const bool lhsMoved = lhs.preCollision->movedThisFrame;
			const bool rhsMoved = rhs.preCollision->movedThisFrame;
			if (distance > kOverlapEpsilon)
			{
				nx = dx / distance;
				nz = dz / distance;
			}
			else
			{
				ResolveZeroDistanceNormal(lhs, rhs, lhsMoved, rhsMoved, nx, nz);
			}

			const bool lhsDynamic =
				lhs.shape->pushability == BodyPushability::Dynamic;
			const bool rhsDynamic =
				rhs.shape->pushability == BodyPushability::Dynamic;
			if (!lhsDynamic && !rhsDynamic)
			{
				continue;
			}

			float lhsShare = 0.0f;
			float rhsShare = 0.0f;

			if (lhsDynamic && !rhsDynamic)
			{
				lhsShare = 1.0f;
			}
			else if (!lhsDynamic && rhsDynamic)
			{
				rhsShare = 1.0f;
			}
			else if (lhsMoved != rhsMoved)
			{
				lhsShare = lhsMoved ? 1.0f : 0.0f;
				rhsShare = rhsMoved ? 1.0f : 0.0f;
			}
			else
			{
				const float lhsWeight =
					std::max(lhs.shape->overlapYieldWeight, kOverlapEpsilon);
				const float rhsWeight =
					std::max(rhs.shape->overlapYieldWeight, kOverlapEpsilon);
				const float weightSum = lhsWeight + rhsWeight;
				lhsShare = lhsWeight / weightSum;
				rhsShare = rhsWeight / weightSum;
			}

			const float correctionX = nx * penetration;
			const float correctionZ = nz * penetration;
			accumulated[i].x -= correctionX * lhsShare;
			accumulated[i].z -= correctionZ * lhsShare;
			accumulated[j].x += correctionX * rhsShare;
			accumulated[j].z += correctionZ * rhsShare;
		}
	}

	auto MarkTransformDirtyIfPresent =
		[&ctx](Entity entity)
		{
			if (DirtyFlagsComp* dirty =
				ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity))
			{
				dirty->MarkDirty(WorldDirtyType::Transform);
			}
		};

	for (size_t i = 0; i < entries.size(); ++i)
	{
		OverlapEntry& entry = entries[i];
		XZCorrection correction = accumulated[i];
		const float maxCorrection =
			std::max(entry.shape->maxOverlapCorrectionPerFrameXZ, 0.0f);
		const float correctionLength = LengthXZ(correction.x, correction.z);
		if (correctionLength > maxCorrection && maxCorrection > kOverlapEpsilon)
		{
			const float scale = maxCorrection / correctionLength;
			correction.x *= scale;
			correction.z *= scale;
		}
		else if (maxCorrection <= kOverlapEpsilon)
		{
			correction = {};
		}

		if (LengthXZ(correction.x, correction.z) <= kOverlapEpsilon)
		{
			continue;
		}

		entry.transform->position.x += correction.x;
		entry.transform->position.z += correction.z;
		entry.resolve->overlapAdjusted = true;
		MarkTransformDirtyIfPresent(entry.entity);
	}
}

const SystemMeta& ResolveCharacterOverlapSystem::Meta() const
{
	return kMeta;
}
