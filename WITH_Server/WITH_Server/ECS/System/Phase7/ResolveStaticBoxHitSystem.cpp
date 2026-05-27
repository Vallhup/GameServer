#include "pch.h"
#include "ResolveStaticBoxHitSystem.h"

#include "../../GameplayRuntimeComponents.h"
#include "../GameplaySystemUtil.h"
#include "CombatHitResolutionUtil.h"
#include "../../../GameplayContentCatalog.h"
#include "../../../TransformHelper.h"

using namespace GameplaySystemUtil;
using namespace DirectX;

namespace
{
	struct BoxHitResult
	{
		bool hit{ false };
		float capsuleT{ 0.0f };
		XMFLOAT3 impactPoint{ 0.0f, 0.0f, 0.0f };
	};

	bool IsExplicitSourceHitBone(
		const AbilityCombatWindowDef& attackWindow,
		uint16_t boneIndex) noexcept
	{
		return std::find(
			attackWindow.sourceHitBones.begin(),
			attackWindow.sourceHitBones.end(),
			boneIndex) != attackWindow.sourceHitBones.end();
	}

	bool PassesSpatialFilter(
		const ECSView& ecs,
		Entity attacker,
		const AbilityStateComp& attackerAbility,
		const WorldTransformComp& attackerTransform,
		const WorldTransformComp& targetTransform,
		const AbilityCombatSpatialFilterDef& spatialFilter) noexcept
	{
		const float dx = targetTransform.position.x - attackerTransform.position.x;
		const float dz = targetTransform.position.z - attackerTransform.position.z;
		const float distanceXZ = std::sqrt(dx * dx + dz * dz);

		if (spatialFilter.minDistance.has_value() &&
			distanceXZ < *spatialFilter.minDistance)
		{
			return false;
		}
		if (spatialFilter.maxDistance.has_value() &&
			distanceXZ > *spatialFilter.maxDistance)
		{
			return false;
		}
		if (spatialFilter.verticalTolerance.has_value())
		{
			const float verticalDelta = std::abs(
				attackerTransform.position.y - targetTransform.position.y);
			if (verticalDelta > *spatialFilter.verticalTolerance)
				return false;
		}
		if (!spatialFilter.facingHalfAngleDeg.has_value())
			return true;

		float toTargetX = dx;
		float toTargetZ = dz;
		if (!TransformHelper::NormalizeXZ(toTargetX, toTargetZ))
			return false;

		XMFLOAT3 referenceDirection{};
		if (!CombatHitResolutionUtil::BuildReferenceDirection(
				ecs,
				attacker,
				attackerAbility,
				attackerTransform,
				spatialFilter.referenceFrame,
				referenceDirection))
		{
			return false;
		}

		const float dot =
			referenceDirection.x * toTargetX +
			referenceDirection.z * toTargetZ;
		const float cosThreshold = std::cos(
			*spatialFilter.facingHalfAngleDeg * (XM_PI / 180.0f));
		return dot >= cosThreshold;
	}

	XMFLOAT3 ToBoxLocal(
		const XMFLOAT3& point,
		const WorldTransformComp& boxTransform,
		const XMMATRIX& inverseBoxWorld)
	{
		(void)boxTransform;
		return TransformHelper::TransformPoint(inverseBoxWorld, point);
	}

	XMFLOAT3 ToBoxWorld(
		const XMFLOAT3& point,
		const XMMATRIX& boxWorld)
	{
		return TransformHelper::TransformPoint(boxWorld, point);
	}

	float SegmentAabbDistanceSq(
		const XMFLOAT3& p0,
		const XMFLOAT3& p1,
		const XMFLOAT3& halfExtents,
		float& outT,
		XMFLOAT3& outClosestBoxPoint)
	{
		constexpr int kSampleCount = 8;
		float bestDistSq = std::numeric_limits<float>::max();
		float bestT = 0.0f;
		XMFLOAT3 bestPoint{};

		for (int i = 0; i <= kSampleCount; ++i)
		{
			const float t = static_cast<float>(i) /
				static_cast<float>(kSampleCount);
			const XMFLOAT3 p{
				p0.x + (p1.x - p0.x) * t,
				p0.y + (p1.y - p0.y) * t,
				p0.z + (p1.z - p0.z) * t
			};
			const XMFLOAT3 clamped{
				std::clamp(p.x, -halfExtents.x, halfExtents.x),
				std::clamp(p.y, -halfExtents.y, halfExtents.y),
				std::clamp(p.z, -halfExtents.z, halfExtents.z)
			};
			const XMFLOAT3 delta = TransformHelper::Subtract(p, clamped);
			const float distSq = TransformHelper::DotF(delta, delta);
			if (distSq < bestDistSq)
			{
				bestDistSq = distSq;
				bestT = t;
				bestPoint = clamped;
			}
		}

		outT = bestT;
		outClosestBoxPoint = bestPoint;
		return bestDistSq;
	}

	BoxHitResult TestCapsuleAgainstBox(
		const Capsule& capsule,
		float radius,
		const WorldTransformComp& boxTransform,
		const StaticBoxHurtColliderComp& box)
	{
		const XMMATRIX boxWorld = TransformHelper::ToMatrix(boxTransform);
		const XMMATRIX inverseBoxWorld = XMMatrixInverse(nullptr, boxWorld);

		const XMFLOAT3 localP0 =
			ToBoxLocal(capsule.p0, boxTransform, inverseBoxWorld);
		const XMFLOAT3 localP1 =
			ToBoxLocal(capsule.p1, boxTransform, inverseBoxWorld);

		float t = 0.0f;
		XMFLOAT3 closestBoxPoint{};
		const float distSq = SegmentAabbDistanceSq(
			localP0,
			localP1,
			box.halfExtents,
			t,
			closestBoxPoint);
		if (distSq > radius * radius)
			return {};

		return BoxHitResult{
			.hit = true,
			.capsuleT = t,
			.impactPoint = ToBoxWorld(closestBoxPoint, boxWorld)
		};
	}

	void AppendStaticBoxInteraction(
		ECSView& ecs,
		Entity attacker,
		Entity object,
		const PendingCombatInteractionRecord& interaction)
	{
		PendingCombatResultComp* objectResult =
			ecs.GetMutableComponent<PendingCombatResultComp>(object);
		if (objectResult == nullptr)
			return;

		objectResult->receivedInteractions.push_back(interaction);
		objectResult->reactionSource = attacker;
		objectResult->wasHitThisFrame = true;
		objectResult->reactionKind = CombatReactionKind::HitReaction;

		if (PendingCombatResultComp* attackerResult =
			ecs.GetMutableComponent<PendingCombatResultComp>(attacker))
		{
			attackerResult->hitAnyVictimThisFrame = true;
		}
	}
}

const StaticSystemMetaStorage<13> ResolveStaticBoxHitSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<ResolveStaticBoxHitSystem>(),
		"ResolveStaticBoxHitSystem",
		std::array<AccessSpec, 13>
	{
		ReadImmediate(ComponentRes<PlayerControlIdentityComp>()),
		ReadImmediate(ComponentRes<CombatColliderActivationComp>()),
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<SkeletalCombatColliderComp>()),
		WriteImmediate(ComponentRes<CombatHitDedupStateComp>()),
		WriteImmediate(ComponentRes<PendingCombatResultComp>()),
		WriteImmediate(ComponentRes<GimmickObjectComp>()),
		ReadImmediate(ComponentRes<StaticBoxHurtColliderComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		ReadImmediate(ExternalRes<AbilityDef>()),
		WriteImmediate(ComponentRes<ReplicationStatsComp>()),
	});

void ResolveStaticBoxHitSystem::Execute(SystemContext& ctx)
{
	std::vector<Entity> attackers;
	for (auto [entity, player, activation] :
		ctx.ecs.View<PlayerControlIdentityComp, CombatColliderActivationComp>())
	{
		if (player.ownerSessionId != 0 &&
			activation.hasAttackWindow &&
			!HasBlockingPendingState(ctx.ecs, entity))
		{
			attackers.push_back(entity);
		}
	}
	std::sort(attackers.begin(), attackers.end(), [](Entity lhs, Entity rhs) {
		return lhs.id < rhs.id;
	});

	for (Entity attacker : attackers)
	{
		AbilityStateComp* attackerAbility =
			ctx.ecs.GetMutableComponent<AbilityStateComp>(attacker);
		WorldTransformComp* attackerTransform =
			ctx.ecs.GetMutableComponent<WorldTransformComp>(attacker);
		SkeletalCombatColliderComp* attackerColliders =
			ctx.ecs.GetMutableComponent<SkeletalCombatColliderComp>(attacker);
		CombatHitDedupStateComp* attackerDedup =
			ctx.ecs.GetMutableComponent<CombatHitDedupStateComp>(attacker);
		if (attackerAbility == nullptr ||
			attackerTransform == nullptr ||
			attackerColliders == nullptr ||
			attackerColliders->localColliders.empty())
		{
			continue;
		}

		if (attackerDedup != nullptr &&
			attackerDedup->boundAbilityInstanceId !=
				attackerAbility->abilityInstanceId)
		{
			attackerDedup->boundAbilityInstanceId =
				attackerAbility->abilityInstanceId;
			attackerDedup->resolvedVictims.clear();
		}

		const AbilityDef* abilityDef =
			GameplayContentCatalogSnapshot::Current()
				.Abilities()
				.Find(attackerAbility->abilityId);
		if (abilityDef == nullptr || abilityDef->timeline.durationSec <= 0.0f)
			continue;

		uint16_t windowIndex = 0;
		const float normalizedTime = ClampFloat(
			attackerAbility->elapsedSec / abilityDef->timeline.durationSec,
			0.0f,
			1.0f);
		const std::optional<AbilityAttackHitDef> attackEffect =
			FindCurrentAttackEffect(*abilityDef, normalizedTime, windowIndex);
		if (!attackEffect.has_value() ||
			windowIndex >= abilityDef->timeline.combatWindows.size())
		{
			continue;
		}

		const AbilityCombatWindowDef& attackWindow =
			abilityDef->timeline.combatWindows[windowIndex];
		const XMMATRIX attackerWorld =
			TransformHelper::ToMatrix(*attackerTransform);
		const float attackerRadiusScale =
			TransformHelper::MaxScaleComponent(*attackerTransform);

		for (auto [object, objectTransform, objectBox, objectGimmick] :
			ctx.ecs.View<
				WorldTransformComp,
				StaticBoxHurtColliderComp,
				GimmickObjectComp>())
		{
			if (objectGimmick.broken ||
				HasBlockingPendingState(ctx.ecs, object))
			{
				continue;
			}

			if (attackerDedup != nullptr &&
				std::find_if(
					attackerDedup->resolvedVictims.begin(),
					attackerDedup->resolvedVictims.end(),
					[object, windowIndex](const CombatHitResolvedVictim& resolved)
					{
						return resolved.victim == object &&
							resolved.attackWindowIndex == windowIndex;
					}) != attackerDedup->resolvedVictims.end())
			{
				continue;
			}

			if (attackWindow.spatialFilter.has_value() &&
				!PassesSpatialFilter(
					ctx.ecs,
					attacker,
					*attackerAbility,
					*attackerTransform,
					objectTransform,
					*attackWindow.spatialFilter))
			{
				continue;
			}

			bool foundHit = false;
			uint16_t bestSourceColliderIndex = InvalidCombatColliderIndex;
			BoxHitResult bestHit{};
			for (uint16_t sourceColliderIndex = 0;
				sourceColliderIndex < attackerColliders->localColliders.size();
				++sourceColliderIndex)
			{
				const SkeletalCombatCollider& sourceCollider =
					attackerColliders->localColliders[sourceColliderIndex];
				const bool explicitSourceHitBone =
					!attackWindow.sourceHitBones.empty() &&
					IsExplicitSourceHitBone(
						attackWindow,
						sourceCollider.boneIndex);
				if (!explicitSourceHitBone &&
					!CombatHitResolutionUtil::HasRole(
						sourceCollider.roleMask,
						SkeletalCombatColliderRoleMask::Hit))
				{
					continue;
				}

				const Capsule worldSourceCapsule{
					TransformHelper::TransformPoint(
						attackerWorld,
						sourceCollider.capsule.p0),
					TransformHelper::TransformPoint(
						attackerWorld,
						sourceCollider.capsule.p1)
				};
				const float worldSourceRadius =
					sourceCollider.radius * attackerRadiusScale;

				const BoxHitResult hit = TestCapsuleAgainstBox(
					worldSourceCapsule,
					worldSourceRadius,
					objectTransform,
					objectBox);
				if (!hit.hit)
					continue;

				if (!foundHit || hit.capsuleT < bestHit.capsuleT)
				{
					foundHit = true;
					bestSourceColliderIndex = sourceColliderIndex;
					bestHit = hit;
				}
			}

			if (!foundHit)
				continue;

			XMFLOAT3 swingDirection{
				objectTransform.position.x - attackerTransform->position.x,
				objectTransform.position.y - attackerTransform->position.y,
				objectTransform.position.z - attackerTransform->position.z
			};
			if (!TransformHelper::TryNormalize(swingDirection))
			{
				(void)CombatHitResolutionUtil::BuildReferenceDirection(
					ctx.ecs,
					attacker,
					*attackerAbility,
					*attackerTransform,
					AbilityCombatReferenceFrame::LockedActionDirection,
					swingDirection);
			}

			PendingCombatInteractionRecord interaction{
				.sourceEntity = attacker,
				.sourceProxyEntity = Entity::Null(),
				.sourceKind = CombatHitSourceKind::SkeletalCollider,
				.sourceAbilityId = attackerAbility->abilityId,
				.sourceAbilityInstanceId =
					attackerAbility->abilityInstanceId,
				.sourceAttackWindowIndex = windowIndex,
				.sourceColliderIndex = bestSourceColliderIndex,
				.targetColliderIndex = InvalidCombatColliderIndex,
				.resultType = CombatResolveResultType::Hit,
				.attackEffect = *attackEffect,
				.maxKnockbackDistance = attackEffect->knockbackDistance,
				.maxHitStopSec = attackEffect->hitStopSec,
				.impactPoint = bestHit.impactPoint,
				.swingDirection = swingDirection
			};
			AppendStaticBoxInteraction(ctx.ecs, attacker, object, interaction);
			objectGimmick.lastHitBy = attacker;

			if (attackerDedup != nullptr)
			{
				attackerDedup->resolvedVictims.push_back(CombatHitResolvedVictim{
					object,
					windowIndex
				});
			}
		}
	}
}
