#pragma once

#include <algorithm>
#include <cmath>
#include <optional>

#include "../../../TransformHelper.h"
#include "../../GameplayRuntimeComponents.h"
#include "../GameplaySystemUtil.h"

namespace CombatHitResolutionUtil
{
	struct SegmentClosestResult
	{
		float lhsT{ 0.0f };
		float rhsT{ 0.0f };
		XMFLOAT3 lhsPoint{ 0.0f, 0.0f, 0.0f };
		XMFLOAT3 rhsPoint{ 0.0f, 0.0f, 0.0f };
		float distanceSq{ 0.0f };
	};

	inline bool HasRole(
		uint8_t roleMask,
		SkeletalCombatColliderRoleMask role) noexcept
	{
		return (roleMask & static_cast<uint8_t>(role)) != 0;
	}

	inline XMFLOAT3 PointOnSegment(const Capsule& capsule, float t) noexcept
	{
		return XMFLOAT3{
			capsule.p0.x + (capsule.p1.x - capsule.p0.x) * t,
			capsule.p0.y + (capsule.p1.y - capsule.p0.y) * t,
			capsule.p0.z + (capsule.p1.z - capsule.p0.z) * t
		};
	}

	inline SegmentClosestResult ComputeSegmentClosestPoints(
		const Capsule& lhs,
		const Capsule& rhs) noexcept
	{
		const XMFLOAT3 u{
			lhs.p1.x - lhs.p0.x,
			lhs.p1.y - lhs.p0.y,
			lhs.p1.z - lhs.p0.z
		};
		const XMFLOAT3 v{
			rhs.p1.x - rhs.p0.x,
			rhs.p1.y - rhs.p0.y,
			rhs.p1.z - rhs.p0.z
		};
		const XMFLOAT3 w{
			lhs.p0.x - rhs.p0.x,
			lhs.p0.y - rhs.p0.y,
			lhs.p0.z - rhs.p0.z
		};

		const float a = TransformHelper::DotF(u, u);
		const float b = TransformHelper::DotF(u, v);
		const float c = TransformHelper::DotF(v, v);
		const float d = TransformHelper::DotF(u, w);
		const float e = TransformHelper::DotF(v, w);
		const float determinant = a * c - b * b;
		const float epsilon = 1.0e-6f;

		float sNumerator = 0.0f;
		float sDenominator = determinant;
		float tNumerator = 0.0f;
		float tDenominator = determinant;

		if (determinant <= epsilon)
		{
			sNumerator = 0.0f;
			sDenominator = 1.0f;
			tNumerator = e;
			tDenominator = c;
		}
		else
		{
			sNumerator = (b * e - c * d);
			tNumerator = (a * e - b * d);

			if (sNumerator < 0.0f)
			{
				sNumerator = 0.0f;
				tNumerator = e;
				tDenominator = c;
			}
			else if (sNumerator > sDenominator)
			{
				sNumerator = sDenominator;
				tNumerator = e + b;
				tDenominator = c;
			}
		}

		if (tNumerator < 0.0f)
		{
			tNumerator = 0.0f;
			if (-d < 0.0f)
				sNumerator = 0.0f;
			else if (-d > a)
				sNumerator = sDenominator;
			else
			{
				sNumerator = -d;
				sDenominator = a;
			}
		}
		else if (tNumerator > tDenominator)
		{
			tNumerator = tDenominator;
			if ((-d + b) < 0.0f)
				sNumerator = 0.0f;
			else if ((-d + b) > a)
				sNumerator = sDenominator;
			else
			{
				sNumerator = (-d + b);
				sDenominator = a;
			}
		}

		const float s = (std::abs(sNumerator) <= epsilon)
			? 0.0f
			: sNumerator / std::max(sDenominator, epsilon);
		const float t = (std::abs(tNumerator) <= epsilon)
			? 0.0f
			: tNumerator / std::max(tDenominator, epsilon);

		SegmentClosestResult result{};
		result.lhsT = s;
		result.rhsT = t;
		result.lhsPoint = PointOnSegment(lhs, s);
		result.rhsPoint = PointOnSegment(rhs, t);
		const XMFLOAT3 delta =
			TransformHelper::Subtract(result.lhsPoint, result.rhsPoint);
		result.distanceSq = TransformHelper::DotF(delta, delta);
		return result;
	}

	inline float SegmentSegmentDistanceSq(
		const Capsule& lhs,
		const Capsule& rhs) noexcept
	{
		return ComputeSegmentClosestPoints(lhs, rhs).distanceSq;
	}

	inline bool CapsulesOverlap(
		const Capsule& lhs,
		float lhsRadius,
		const Capsule& rhs,
		float rhsRadius) noexcept
	{
		const float sumRadius = lhsRadius + rhsRadius;
		return SegmentSegmentDistanceSq(lhs, rhs) <= sumRadius * sumRadius;
	}

	inline XMFLOAT3 CapsuleCenter(const Capsule& capsule) noexcept
	{
		return TransformHelper::Scale(
			TransformHelper::Add(capsule.p0, capsule.p1),
			0.5f);
	}

	inline int GetResultPriority(CombatResolveResultType resultType) noexcept
	{
		switch (resultType)
		{
		case CombatResolveResultType::Parry:
			return 3;
		case CombatResolveResultType::Guard:
			return 2;
		case CombatResolveResultType::Hit:
		default:
			return 1;
		}
	}

	inline const AbilityCombatWindowDef* FindActiveCombatWindow(
		const AbilityStateComp& abilityState,
		AbilityCombatWindowKind windowType)
	{
		if (abilityState.abilityId == InvalidAbilityId)
			return nullptr;

		const AbilityDef* abilityDef =
			GameplayContentCatalogSnapshot::Current().Abilities().Find(
				abilityState.abilityId);
		if (abilityDef == nullptr || abilityDef->timeline.durationSec <= 0.0f)
			return nullptr;

		const float normalizedTime = GameplaySystemUtil::ClampFloat(
			abilityState.elapsedSec / abilityDef->timeline.durationSec,
			0.0f,
			1.0f);

		for (const AbilityCombatWindowDef& window :
			abilityDef->timeline.combatWindows)
		{
			if (window.kind == windowType &&
				normalizedTime >= window.startNormalized &&
				normalizedTime < window.endNormalized)
			{
				return &window;
			}
		}
		return nullptr;
	}

	inline bool DoesWindowApplyToAttack(
		const AbilityCombatWindowDef& window,
		const AbilityAttackHitDef& attackEffect) noexcept
	{
		if (!window.appliesTo.has_value())
			return true;

		switch (*window.appliesTo)
		{
		case AbilityCombatApplyTo::ParryableAttack:
			return attackEffect.parryable;
		case AbilityCombatApplyTo::GuardableAttack:
			return attackEffect.guardable;
		case AbilityCombatApplyTo::FrontPhysical:
		default:
			return true;
		}
	}

	inline bool BuildReferenceDirection(
		const ECSView& ecs,
		Entity owner,
		const AbilityStateComp& ownerAbility,
		const WorldTransformComp& ownerTransform,
		AbilityCombatReferenceFrame referenceFrame,
		XMFLOAT3& outDirection) noexcept
	{
		float dirX = 0.0f;
		float dirZ = 0.0f;
		switch (referenceFrame)
		{
		case AbilityCombatReferenceFrame::MoveDirection:
			if (const auto* locomotion =
				ecs.GetComponent<LocomotionStateComp>(owner))
			{
				dirX = locomotion->desiredMoveDirX;
				dirZ = locomotion->desiredMoveDirZ;
			}
			break;
		case AbilityCombatReferenceFrame::LockedActionDirection:
			dirX = ownerAbility.directionX;
			dirZ = ownerAbility.directionZ;
			break;
		case AbilityCombatReferenceFrame::OwnerFacing:
		default:
			break;
		}

		if (!TransformHelper::NormalizeXZ(dirX, dirZ))
		{
			const XMVECTOR facing = TransformHelper::Forward(ownerTransform);
			dirX = XMVectorGetX(facing);
			dirZ = XMVectorGetZ(facing);
			if (!TransformHelper::NormalizeXZ(dirX, dirZ))
				return false;
		}

		outDirection = XMFLOAT3{ dirX, 0.0f, dirZ };
		return true;
	}

	inline bool PassesDefensiveSpatialFilter(
		const ECSView& ecs,
		Entity defender,
		const AbilityStateComp& defenderAbility,
		const WorldTransformComp& defenderTransform,
		const XMFLOAT3& incomingSourcePoint,
		const AbilityCombatSpatialFilterDef& spatialFilter) noexcept
	{
		const float dx = incomingSourcePoint.x - defenderTransform.position.x;
		const float dz = incomingSourcePoint.z - defenderTransform.position.z;
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
			const float verticalDelta =
				std::abs(incomingSourcePoint.y - defenderTransform.position.y);
			if (verticalDelta > *spatialFilter.verticalTolerance)
				return false;
		}
		if (!spatialFilter.facingHalfAngleDeg.has_value())
			return true;

		float toSourceX = dx;
		float toSourceZ = dz;
		if (!TransformHelper::NormalizeXZ(toSourceX, toSourceZ))
			return false;

		XMFLOAT3 referenceDirection{};
		if (!BuildReferenceDirection(
				ecs,
				defender,
				defenderAbility,
				defenderTransform,
				spatialFilter.referenceFrame,
				referenceDirection))
		{
			return false;
		}

		const float dot =
			referenceDirection.x * toSourceX +
			referenceDirection.z * toSourceZ;
		const float cosThreshold = std::cos(
			*spatialFilter.facingHalfAngleDeg * (XM_PI / 180.0f));
		return dot >= cosThreshold;
	}

	inline bool TryResolveDefensiveEffects(
		const AbilityStateComp& victimAbility,
		CombatResolveResultType resultType,
		std::optional<AbilityGuardResponseDef>& outGuardEffect,
		std::optional<AbilityParryResponseDef>& outParryEffect,
		float& outMaxHitStopSec)
	{
		outGuardEffect.reset();
		outParryEffect.reset();
		outMaxHitStopSec = 0.0f;

		if (victimAbility.abilityId == InvalidAbilityId)
			return true;

		const AbilityDef* victimAbilityDef =
			GameplayContentCatalogSnapshot::Current().Abilities().Find(
				victimAbility.abilityId);
		if (victimAbilityDef == nullptr ||
			victimAbilityDef->timeline.durationSec <= 0.0f)
		{
			return true;
		}

		const float normalizedTime = GameplaySystemUtil::ClampFloat(
			victimAbility.elapsedSec / victimAbilityDef->timeline.durationSec,
			0.0f,
			1.0f);

		for (const AbilityCombatWindowDef& window :
			victimAbilityDef->timeline.combatWindows)
		{
			if (normalizedTime < window.startNormalized ||
				normalizedTime >= window.endNormalized ||
				!window.effect.has_value())
			{
				continue;
			}

			if (resultType == CombatResolveResultType::Guard &&
				window.kind == AbilityCombatWindowKind::Guard &&
				window.effect->guardResponse.has_value())
			{
				outGuardEffect = window.effect->guardResponse;
				outMaxHitStopSec = outGuardEffect->hitStopSec;
				return true;
			}
			if (resultType == CombatResolveResultType::Parry &&
				window.kind == AbilityCombatWindowKind::Parry &&
				window.effect->parryResponse.has_value())
			{
				outParryEffect = window.effect->parryResponse;
				outMaxHitStopSec = outParryEffect->hitStopSec;
				return true;
			}
		}
		return true;
	}

	template<typename TOverlapPredicate>
	inline bool TryBuildNonSkeletalInteractionRecord(
		const ECSView& ecs,
		Entity source,
		Entity sourceProxy,
		CombatHitSourceKind sourceKind,
		AbilityId sourceAbilityId,
		uint32_t sourceAbilityInstanceId,
		uint16_t sourceEventIndex,
		const AbilityAttackHitDef& attackEffect,
		const XMFLOAT3& incomingSourcePoint,
		const XMFLOAT3& swingDirection,
		Entity victim,
		const AbilityStateComp& victimAbility,
		const WorldTransformComp& victimTransform,
		const SkeletalCombatColliderComp& victimColliders,
		const CombatColliderActivationComp& victimActivation,
		TOverlapPredicate overlaps,
		PendingCombatInteractionRecord& outRecord)
	{
		if (victimColliders.localColliders.empty())
			return false;

		const XMMATRIX victimWorldMatrix =
			TransformHelper::ToMatrix(victimTransform);
		const float victimRadiusScale =
			TransformHelper::MaxScaleComponent(victimTransform);

		int bestPriority = 0;
		bool foundInteraction = false;
		uint16_t bestTargetColliderIndex = InvalidCombatColliderIndex;
		CombatResolveResultType bestResultType = CombatResolveResultType::Hit;
		XMFLOAT3 bestImpactPoint = victimTransform.position;

		for (uint16_t targetColliderIndex = 0;
			targetColliderIndex < victimColliders.localColliders.size();
			++targetColliderIndex)
		{
			const SkeletalCombatCollider& targetCollider =
				victimColliders.localColliders[targetColliderIndex];

			CombatResolveResultType candidateResultType =
				CombatResolveResultType::Hit;
			bool canReceiveHit = false;

			if (victimActivation.hasParryWindow &&
				attackEffect.parryable &&
				HasRole(targetCollider.roleMask, SkeletalCombatColliderRoleMask::Parry))
			{
				const AbilityCombatWindowDef* parryWindow =
					FindActiveCombatWindow(victimAbility, AbilityCombatWindowKind::Parry);
				if (parryWindow != nullptr &&
					DoesWindowApplyToAttack(*parryWindow, attackEffect) &&
					(!parryWindow->spatialFilter.has_value() ||
						PassesDefensiveSpatialFilter(
							ecs,
							victim,
							victimAbility,
							victimTransform,
							incomingSourcePoint,
							*parryWindow->spatialFilter)))
				{
					candidateResultType = CombatResolveResultType::Parry;
					canReceiveHit = true;
				}
			}
			if (!canReceiveHit &&
				victimActivation.hasGuardWindow &&
				attackEffect.guardable &&
				HasRole(targetCollider.roleMask, SkeletalCombatColliderRoleMask::Guard))
			{
				const AbilityCombatWindowDef* guardWindow =
					FindActiveCombatWindow(victimAbility, AbilityCombatWindowKind::Guard);
				if (guardWindow != nullptr &&
					DoesWindowApplyToAttack(*guardWindow, attackEffect) &&
					(!guardWindow->spatialFilter.has_value() ||
						PassesDefensiveSpatialFilter(
							ecs,
							victim,
							victimAbility,
							victimTransform,
							incomingSourcePoint,
							*guardWindow->spatialFilter)))
				{
					candidateResultType = CombatResolveResultType::Guard;
					canReceiveHit = true;
				}
			}
			if (!canReceiveHit &&
				HasRole(targetCollider.roleMask, SkeletalCombatColliderRoleMask::Hurt))
			{
				candidateResultType = CombatResolveResultType::Hit;
				canReceiveHit = true;
			}
			if (!canReceiveHit)
				continue;

			const Capsule worldTargetCapsule{
				TransformHelper::TransformPoint(victimWorldMatrix, targetCollider.capsule.p0),
				TransformHelper::TransformPoint(victimWorldMatrix, targetCollider.capsule.p1)
			};
			const float worldTargetRadius =
				targetCollider.radius * victimRadiusScale;

			XMFLOAT3 impactPoint = CapsuleCenter(worldTargetCapsule);
			if (!overlaps(
					worldTargetCapsule,
					worldTargetRadius,
					candidateResultType,
					impactPoint))
			{
				continue;
			}

			const int candidatePriority = GetResultPriority(candidateResultType);
			if (candidatePriority <= bestPriority)
				continue;

			bestPriority = candidatePriority;
			bestTargetColliderIndex = targetColliderIndex;
			bestResultType = candidateResultType;
			bestImpactPoint = impactPoint;
			foundInteraction = true;
			if (candidateResultType == CombatResolveResultType::Parry)
				break;
		}

		if (!foundInteraction)
			return false;

		std::optional<AbilityGuardResponseDef> guardEffect;
		std::optional<AbilityParryResponseDef> parryEffect;
		float maxHitStopSec = attackEffect.hitStopSec;
		TryResolveDefensiveEffects(
			victimAbility,
			bestResultType,
			guardEffect,
			parryEffect,
			maxHitStopSec);
		maxHitStopSec = std::max(maxHitStopSec, attackEffect.hitStopSec);

		XMFLOAT3 resolvedSwingDirection = swingDirection;
		if (!TransformHelper::TryNormalize(resolvedSwingDirection))
		{
			resolvedSwingDirection =
				TransformHelper::Subtract(victimTransform.position, incomingSourcePoint);
			if (!TransformHelper::TryNormalize(resolvedSwingDirection))
				resolvedSwingDirection = XMFLOAT3{ 0.0f, 0.0f, -1.0f };
		}

		outRecord = PendingCombatInteractionRecord{
			.sourceEntity = source,
			.sourceProxyEntity = sourceProxy,
			.sourceKind = sourceKind,
			.sourceAbilityId = sourceAbilityId,
			.sourceAbilityInstanceId = sourceAbilityInstanceId,
			.sourceAttackWindowIndex = sourceEventIndex,
			.sourceColliderIndex = InvalidCombatColliderIndex,
			.targetColliderIndex = bestTargetColliderIndex,
			.resultType = bestResultType,
			.attackEffect = attackEffect,
			.guardEffect = guardEffect,
			.parryEffect = parryEffect,
			.maxKnockbackDistance = attackEffect.knockbackDistance,
			.maxHitStopSec = maxHitStopSec,
			.impactPoint = bestImpactPoint,
			.swingDirection = resolvedSwingDirection
		};
		return true;
	}

	inline void AppendInteractionResult(
		ECSView& ecs,
		Entity source,
		Entity victim,
		const PendingCombatInteractionRecord& interaction)
	{
		PendingCombatResultComp* victimResult =
			ecs.GetMutableComponent<PendingCombatResultComp>(victim);
		if (victimResult == nullptr)
			return;

		PendingCombatResultComp* sourceResult =
			ecs.GetMutableComponent<PendingCombatResultComp>(source);

		victimResult->receivedInteractions.push_back(interaction);
		if (PendingCombatImpactEventComp* impactEvents =
			ecs.GetMutableComponent<PendingCombatImpactEventComp>(victim))
		{
			impactEvents->events.push_back(PendingCombatImpactEvent{
				.sourceEntity = source,
				.sourceProxyEntity = interaction.sourceProxyEntity,
				.targetEntity = victim,
				.sourceKind = interaction.sourceKind,
				.sourceAbilityId = interaction.sourceAbilityId,
				.sourceAbilityInstanceId = interaction.sourceAbilityInstanceId,
				.sourceAttackWindowIndex = interaction.sourceAttackWindowIndex,
				.sourceColliderIndex = interaction.sourceColliderIndex,
				.targetColliderIndex = interaction.targetColliderIndex,
				.resultType = interaction.resultType,
				.impactPoint = interaction.impactPoint,
				.swingDirection = interaction.swingDirection
			});
		}

		victimResult->reactionSource = source;
		victimResult->wasHitThisFrame |=
			interaction.resultType == CombatResolveResultType::Hit;
		victimResult->guardSucceededThisFrame |=
			interaction.resultType == CombatResolveResultType::Guard;
		victimResult->parrySucceededThisFrame |=
			interaction.resultType == CombatResolveResultType::Parry;
		if (interaction.resultType == CombatResolveResultType::Hit)
			victimResult->reactionKind = CombatReactionKind::HitReaction;

		if (sourceResult != nullptr)
		{
			sourceResult->hitAnyVictimThisFrame |=
				interaction.resultType == CombatResolveResultType::Hit;
			sourceResult->parriedByAnyVictimThisFrame |=
				interaction.resultType == CombatResolveResultType::Parry;
		}
	}
}
