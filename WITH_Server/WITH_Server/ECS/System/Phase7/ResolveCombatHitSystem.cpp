#include "pch.h"
#include "ResolveCombatHitSystem.h"

#include "../../GameplayRuntimeComponents.h"
#include "../GameplaySystemUtil.h"
#include "../../../TransformHelper.h"

using namespace GameplaySystemUtil;
using namespace DirectX;

namespace
{
	struct SegmentClosestResult
	{
		float lhsT{ 0.0f };
		float rhsT{ 0.0f };
		XMFLOAT3 lhsPoint{ 0.0f, 0.0f, 0.0f };
		XMFLOAT3 rhsPoint{ 0.0f, 0.0f, 0.0f };
		float distanceSq{ 0.0f };
	};

	XMFLOAT3 PointOnSegment(const Capsule& capsule, float t) noexcept
	{
		return XMFLOAT3{
			capsule.p0.x + (capsule.p1.x - capsule.p0.x) * t,
			capsule.p0.y + (capsule.p1.y - capsule.p0.y) * t,
			capsule.p0.z + (capsule.p1.z - capsule.p0.z) * t
		};
	}

	SegmentClosestResult ComputeSegmentClosestPoints(
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
			{
				sNumerator = 0.0f;
			}
			else if (-d > a)
			{
				sNumerator = sDenominator;
			}
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
			{
				sNumerator = 0.0f;
			}
			else if ((-d + b) > a)
			{
				sNumerator = sDenominator;
			}
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
		result.distanceSq = TransformHelper::DotF(
			TransformHelper::Subtract(result.lhsPoint, result.rhsPoint),
			TransformHelper::Subtract(result.lhsPoint, result.rhsPoint));
		return result;
	}
}

const StaticSystemMetaStorage<12> ResolveCombatHitSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<ResolveCombatHitSystem>(),
		"ResolveCombatHitSystem",
		std::array<AccessSpec, 12>
	{
		ReadImmediate(ComponentRes<CombatColliderActivationComp>()),
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<SkeletalCombatColliderComp>()),
		WriteImmediate(ComponentRes<CombatHitDedupStateComp>()),
		WriteImmediate(ComponentRes<PendingCombatResultComp>()),
		WriteImmediate(ComponentRes<PendingCombatImpactEventComp>()),
		ReadImmediate(ComponentRes<LocomotionStateComp>()),
		ReadImmediate(ComponentRes<SpawnTypeComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		ReadImmediate(ExternalRes<AbilityDef>()),
	});

void ResolveCombatHitSystem::Execute(SystemContext& ctx)
{
	std::vector<Entity> attackers;
	for (auto [entity, activation] :
		ctx.ecs.View<CombatColliderActivationComp>())
	{
		if (activation.hasAttackWindow &&
			!HasBlockingPendingState(ctx.ecs, entity))
		{
			attackers.push_back(entity);
		}
	}

	std::sort(
		attackers.begin(),
		attackers.end(),
		[](Entity lhs, Entity rhs)
		{
			return lhs.id < rhs.id;
		});

	for (Entity attacker : attackers)
	{
		auto* attackerAbility =
			ctx.ecs.GetMutableComponent<AbilityStateComp>(attacker);
		auto* attackerTransform =
			ctx.ecs.GetMutableComponent<WorldTransformComp>(attacker);
		auto* attackerColliders =
			ctx.ecs.GetMutableComponent<SkeletalCombatColliderComp>(attacker);
		auto* attackerDedup =
			ctx.ecs.GetMutableComponent<CombatHitDedupStateComp>(attacker);
		if (attackerAbility == nullptr || attackerTransform == nullptr ||
			attackerColliders == nullptr)
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
			GameplayContentCatalogSnapshot::Current().Abilities().Find(attackerAbility->abilityId);
		if (abilityDef == nullptr || abilityDef->timeline.durationSec <= 0.0f)
		{
			continue;
		}

		uint16_t windowIndex = 0;
		const float normalizedTime = ClampFloat(
			attackerAbility->elapsedSec / abilityDef->timeline.durationSec,
			0.0f,
			1.0f);
		const std::optional<AbilityAttackHitDef> attackEffect =
			FindCurrentAttackEffect(*abilityDef, normalizedTime, windowIndex);
		if (!attackEffect.has_value())
		{
			continue;
		}

		for (auto [victim, victimAbility, victimTransform, victimColliders, victimActivation] :
			ctx.ecs.View<
				AbilityStateComp,
				WorldTransformComp,
				SkeletalCombatColliderComp,
				CombatColliderActivationComp>())
		{
			if (victim == attacker ||
				HasBlockingPendingState(ctx.ecs, victim) ||
				IsSameFaction(ctx.ecs, attacker, victim) ||
				victimActivation.hasInvulnerabilityWindow)
			{
				continue;
			}

			if (attackerDedup != nullptr &&
				std::find(
					attackerDedup->resolvedVictims.begin(),
					attackerDedup->resolvedVictims.end(),
					victim) != attackerDedup->resolvedVictims.end())
			{
				continue;
			}

			PendingCombatResultComp* victimResult =
				ctx.ecs.GetMutableComponent<PendingCombatResultComp>(victim);
			if (victimResult == nullptr)
			{
				continue;
			}

			PendingCombatResultComp* attackerResult =
				ctx.ecs.GetMutableComponent<PendingCombatResultComp>(attacker);

			PendingCombatInteractionRecord interaction{};
			if (!TryBuildInteractionRecord(
				ctx.ecs,
				attacker,
				*attackerAbility,
				*attackerTransform,
				*attackerColliders,
				*attackEffect,
				windowIndex,
				victim,
				victimAbility,
				victimTransform,
				victimColliders,
				victimActivation,
				interaction))
			{
				continue;
			}

			victimResult->receivedInteractions.push_back(interaction);
			if (PendingCombatImpactEventComp* impactEvents =
				ctx.ecs.GetMutableComponent<PendingCombatImpactEventComp>(victim))
			{
				impactEvents->events.push_back(PendingCombatImpactEvent{
					.sourceEntity = attacker,
					.targetEntity = victim,
					.sourceAbilityId = interaction.sourceAbilityId,
					.sourceAbilityInstanceId =
						interaction.sourceAbilityInstanceId,
					.sourceAttackWindowIndex =
						interaction.sourceAttackWindowIndex,
					.sourceColliderIndex = interaction.sourceColliderIndex,
					.targetColliderIndex = interaction.targetColliderIndex,
					.resultType = interaction.resultType,
					.impactPoint = interaction.impactPoint,
					.swingDirection = interaction.swingDirection
				});
			}
			victimResult->reactionSource = attacker;
			victimResult->wasHitThisFrame |=
				interaction.resultType == CombatResolveResultType::Hit;
			victimResult->guardSucceededThisFrame |=
				interaction.resultType == CombatResolveResultType::Guard;
			victimResult->parrySucceededThisFrame |=
				interaction.resultType == CombatResolveResultType::Parry;
			if (interaction.resultType == CombatResolveResultType::Hit)
			{
				victimResult->reactionKind = CombatReactionKind::HitReaction;
			}
			if (attackerResult != nullptr)
			{
				attackerResult->hitAnyVictimThisFrame |=
					interaction.resultType == CombatResolveResultType::Hit;
				attackerResult->parriedByAnyVictimThisFrame |=
					interaction.resultType == CombatResolveResultType::Parry;
			}

			if (attackerDedup != nullptr)
			{
				attackerDedup->resolvedVictims.push_back(victim);
			}
		}
	}
}

bool ResolveCombatHitSystem::HasRole(
	uint8_t roleMask,
	SkeletalCombatColliderRoleMask role) noexcept
{
	return (roleMask & static_cast<uint8_t>(role)) != 0;
}

float ResolveCombatHitSystem::SegmentSegmentDistanceSq(
	const Capsule& lhs,
	const Capsule& rhs) noexcept
{
	return ComputeSegmentClosestPoints(lhs, rhs).distanceSq;
}

bool ResolveCombatHitSystem::CapsulesOverlap(
	const Capsule& lhs,
	float lhsRadius,
	const Capsule& rhs,
	float rhsRadius) noexcept
{
	const float distanceSq = SegmentSegmentDistanceSq(lhs, rhs);
	const float sumRadius = lhsRadius + rhsRadius;
	return distanceSq <= (sumRadius * sumRadius);
}

int ResolveCombatHitSystem::GetResultPriority(
	CombatResolveResultType resultType) noexcept
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

bool ResolveCombatHitSystem::TryResolveDefensiveEffects(
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
	{
		return true;
	}

	const AbilityDef* victimAbilityDef =
		GameplayContentCatalogSnapshot::Current().Abilities().Find(victimAbility.abilityId);
	if (victimAbilityDef == nullptr || victimAbilityDef->timeline.durationSec <= 0.0f)
	{
		return true;
	}

	const float normalizedTime = ClampFloat(
		victimAbility.elapsedSec / victimAbilityDef->timeline.durationSec,
		0.0f,
		1.0f);

	for (const AbilityCombatWindowDef& window : victimAbilityDef->timeline.combatWindows)
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

const AbilityCombatWindowDef* ResolveCombatHitSystem::FindActiveCombatWindow(
	const AbilityStateComp& abilityState,
	AbilityCombatWindowKind windowType)
{
	if (abilityState.abilityId == InvalidAbilityId)
	{
		return nullptr;
	}

	const AbilityDef* abilityDef =
		GameplayContentCatalogSnapshot::Current().Abilities().Find(abilityState.abilityId);
	if (abilityDef == nullptr || abilityDef->timeline.durationSec <= 0.0f)
	{
		return nullptr;
	}

	const float normalizedTime = ClampFloat(
		abilityState.elapsedSec / abilityDef->timeline.durationSec,
		0.0f,
		1.0f);

	for (const AbilityCombatWindowDef& window : abilityDef->timeline.combatWindows)
	{
		if (window.kind != windowType ||
			normalizedTime < window.startNormalized ||
			normalizedTime >= window.endNormalized)
		{
			continue;
		}

		return &window;
	}

	return nullptr;
}

bool ResolveCombatHitSystem::DoesWindowApplyToAttack(
	const AbilityCombatWindowDef& window,
	const AbilityAttackHitDef& attackEffect) noexcept
{
	if (!window.appliesTo.has_value())
	{
		return true;
	}

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

bool ResolveCombatHitSystem::BuildReferenceDirection(
	const ECSView& ecs,
	Entity owner,
	const AbilityStateComp& ownerAbility,
	const WorldTransformComp& ownerTransform,
	AbilityCombatReferenceFrame referenceFrame,
	XMFLOAT3& outDirection) noexcept
{
	float dirX = 0.0f;
	float dirZ = 0.0f;
	switch (referenceFrame) {
	case AbilityCombatReferenceFrame::MoveDirection:
	{
		if (const auto* locomotion =
			ecs.GetComponent<LocomotionStateComp>(owner))
		{
			dirX = locomotion->desiredMoveDirX;
			dirZ = locomotion->desiredMoveDirZ;
		}
		break;
	}
	case AbilityCombatReferenceFrame::LockedActionDirection:
	{
		dirX = ownerAbility.directionX;
		dirZ = ownerAbility.directionZ;
		break;
	}
	case AbilityCombatReferenceFrame::OwnerFacing:
	default:
	{
		break;
	}
	}

	if (!TransformHelper::NormalizeXZ(dirX, dirZ))
	{
		const XMVECTOR facing = TransformHelper::Forward(ownerTransform);
		dirX = XMVectorGetX(facing);
		dirZ = XMVectorGetZ(facing);
		if (!TransformHelper::NormalizeXZ(dirX, dirZ))
		{
			return false;
		}
	}

	outDirection = XMFLOAT3{ dirX, 0.0f, dirZ };
	return true;
}

bool ResolveCombatHitSystem::PassesSpatialFilter(
	const ECSView& ecs,
	Entity source,
	const AbilityStateComp& sourceAbility,
	const WorldTransformComp& sourceTransform,
	const WorldTransformComp& targetTransform,
	const AbilityCombatSpatialFilterDef& spatialFilter) noexcept
{
	const float dx = targetTransform.position.x - sourceTransform.position.x;
	const float dz = targetTransform.position.z - sourceTransform.position.z;
	const float distanceSqXZ = dx * dx + dz * dz;
	const float distanceXZ = std::sqrt(distanceSqXZ);

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
			sourceTransform.position.y - targetTransform.position.y);
		if (verticalDelta > *spatialFilter.verticalTolerance)
		{
			return false;
		}
	}

	if (!spatialFilter.facingHalfAngleDeg.has_value())
	{
		return true;
	}

	float toAttackerX = dx;
	float toAttackerZ = dz;
	if (!TransformHelper::NormalizeXZ(toAttackerX, toAttackerZ))
	{
		return false;
	}

	XMFLOAT3 referenceDirection{};
	if (!BuildReferenceDirection(
		ecs,
		source,
		sourceAbility,
		sourceTransform,
		spatialFilter.referenceFrame,
		referenceDirection))
	{
		return false;
	}

	const float dot =
		referenceDirection.x * toAttackerX +
		referenceDirection.z * toAttackerZ;
	const float cosThreshold = std::cos(
		*spatialFilter.facingHalfAngleDeg * (XM_PI / 180.0f));
	return dot >= cosThreshold;
}

bool ResolveCombatHitSystem::TryBuildInteractionRecord(
	const ECSView& ecs,
	Entity attacker,
	const AbilityStateComp& attackerAbility,
	const WorldTransformComp& attackerTransform,
	const SkeletalCombatColliderComp& attackerColliders,
	const AbilityAttackHitDef& attackEffect,
	uint16_t attackWindowIndex,
	Entity victim,
	const AbilityStateComp& victimAbility,
	const WorldTransformComp& victimTransform,
	const SkeletalCombatColliderComp& victimColliders,
	const CombatColliderActivationComp& victimActivation,
	PendingCombatInteractionRecord& outRecord)
{
	if (attackerColliders.localColliders.empty() ||
		victimColliders.localColliders.empty())
	{
		return false;
	}

	const AbilityDef* attackerAbilityDef =
		GameplayContentCatalogSnapshot::Current().Abilities().Find(attackerAbility.abilityId);
	if (attackerAbilityDef == nullptr ||
		attackWindowIndex >= attackerAbilityDef->timeline.combatWindows.size())
	{
		return false;
	}

	const AbilityCombatWindowDef& attackWindow =
		attackerAbilityDef->timeline.combatWindows[attackWindowIndex];
	if (attackWindow.kind != AbilityCombatWindowKind::Attack)
	{
		return false;
	}

	if (attackWindow.spatialFilter.has_value() &&
		!PassesSpatialFilter(
			ecs,
			attacker,
			attackerAbility,
			attackerTransform,
			victimTransform,
			*attackWindow.spatialFilter))
	{
		return false;
	}

	const XMMATRIX attackerWorldMatrix = TransformHelper::ToMatrix(attackerTransform);
	const XMMATRIX victimWorldMatrix   = TransformHelper::ToMatrix(victimTransform);
	const float attackerRadiusScale    = TransformHelper::MaxScaleComponent(attackerTransform);
	const float victimRadiusScale      = TransformHelper::MaxScaleComponent(victimTransform);

	int bestPriority = 0;
	bool foundInteraction = false;
	uint16_t bestSourceColliderIndex = 0;
	uint16_t bestTargetColliderIndex = 0;
	CombatResolveResultType bestResultType = CombatResolveResultType::Hit;

	for (uint16_t sourceColliderIndex = 0;
		sourceColliderIndex < attackerColliders.localColliders.size();
		++sourceColliderIndex)
	{
		const SkeletalCombatCollider& sourceCollider =
			attackerColliders.localColliders[sourceColliderIndex];
		if (!HasRole(sourceCollider.roleMask, SkeletalCombatColliderRoleMask::Hit))
		{
			continue;
		}

		const Capsule worldSourceCapsule{
			TransformHelper::TransformPoint(attackerWorldMatrix, sourceCollider.capsule.p0),
			TransformHelper::TransformPoint(attackerWorldMatrix, sourceCollider.capsule.p1)
		};
		const float worldSourceRadius = sourceCollider.radius * attackerRadiusScale;

		for (uint16_t targetColliderIndex = 0;
			targetColliderIndex < victimColliders.localColliders.size();
			++targetColliderIndex)
		{
			const SkeletalCombatCollider& targetCollider =
				victimColliders.localColliders[targetColliderIndex];

			CombatResolveResultType candidateResultType = CombatResolveResultType::Hit;
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
						PassesSpatialFilter(
							ecs,
							victim,
							victimAbility,
							victimTransform,
							attackerTransform,
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
						PassesSpatialFilter(
							ecs,
							victim,
							victimAbility,
							victimTransform,
							attackerTransform,
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
			{
				continue;
			}

			const Capsule worldTargetCapsule{
				TransformHelper::TransformPoint(victimWorldMatrix, targetCollider.capsule.p0),
				TransformHelper::TransformPoint(victimWorldMatrix, targetCollider.capsule.p1)
			};
			const float worldTargetRadius = targetCollider.radius * victimRadiusScale;
			if (!CapsulesOverlap(
				worldSourceCapsule,
				worldSourceRadius,
				worldTargetCapsule,
				worldTargetRadius))
			{
				continue;
			}

			const int candidatePriority = GetResultPriority(candidateResultType);
			if (candidatePriority <= bestPriority)
			{
				continue;
			}

			bestPriority = candidatePriority;
			bestSourceColliderIndex = sourceColliderIndex;
			bestTargetColliderIndex = targetColliderIndex;
			bestResultType = candidateResultType;
			foundInteraction = true;

			if (candidateResultType == CombatResolveResultType::Parry)
			{
				break;
			}
		}

		if (bestResultType == CombatResolveResultType::Parry)
		{
			break;
		}
	}

	if (!foundInteraction)
	{
		return false;
	}

	const SkeletalCombatCollider& bestSourceCollider =
		attackerColliders.localColliders[bestSourceColliderIndex];
	const SkeletalCombatCollider& bestTargetCollider =
		victimColliders.localColliders[bestTargetColliderIndex];
	const Capsule bestWorldSourceCapsule{
		TransformHelper::TransformPoint(attackerWorldMatrix, bestSourceCollider.capsule.p0),
		TransformHelper::TransformPoint(attackerWorldMatrix, bestSourceCollider.capsule.p1)
	};
	const Capsule bestWorldTargetCapsule{
		TransformHelper::TransformPoint(victimWorldMatrix, bestTargetCollider.capsule.p0),
		TransformHelper::TransformPoint(victimWorldMatrix, bestTargetCollider.capsule.p1)
	};
	const float bestWorldSourceRadius =
		bestSourceCollider.radius * attackerRadiusScale;
	const float bestWorldTargetRadius =
		bestTargetCollider.radius * victimRadiusScale;
	const SegmentClosestResult contact =
		ComputeSegmentClosestPoints(
			bestWorldSourceCapsule,
			bestWorldTargetCapsule);

	XMFLOAT3 contactNormal =
		TransformHelper::Subtract(contact.rhsPoint, contact.lhsPoint);
	if (!TransformHelper::TryNormalize(contactNormal))
	{
		contactNormal = TransformHelper::Subtract(
			victimTransform.position,
			attackerTransform.position);
		if (!TransformHelper::TryNormalize(contactNormal) &&
			!BuildReferenceDirection(
				ecs,
				attacker,
				attackerAbility,
				attackerTransform,
				AbilityCombatReferenceFrame::LockedActionDirection,
				contactNormal))
		{
			contactNormal = XMFLOAT3{ 0.0f, 0.0f, -1.0f };
		}
	}

	const XMFLOAT3 sourceSurfacePoint =
		TransformHelper::Add(contact.lhsPoint, TransformHelper::Scale(contactNormal, bestWorldSourceRadius));
	const XMFLOAT3 targetSurfacePoint =
		TransformHelper::Subtract(contact.rhsPoint, TransformHelper::Scale(contactNormal, bestWorldTargetRadius));
	const XMFLOAT3 impactPoint =
		TransformHelper::Scale(TransformHelper::Add(sourceSurfacePoint, targetSurfacePoint), 0.5f);

	XMFLOAT3 swingDirection{ 0.0f, 0.0f, -1.0f };
	bool hasSwingDirection = false;
	if (attackerColliders.hasPreviousFrameLocalColliders &&
		bestSourceColliderIndex <
			attackerColliders.previousFrameLocalColliders.size())
	{
		const SkeletalCombatCollider& previousSourceCollider =
			attackerColliders.previousFrameLocalColliders[bestSourceColliderIndex];
		const Capsule previousWorldSourceCapsule{
			TransformHelper::TransformPoint(
				attackerWorldMatrix,
				previousSourceCollider.capsule.p0),
			TransformHelper::TransformPoint(
				attackerWorldMatrix,
				previousSourceCollider.capsule.p1)
		};
		swingDirection = TransformHelper::Subtract(
			contact.lhsPoint,
			PointOnSegment(previousWorldSourceCapsule, contact.lhsT));
		hasSwingDirection = TransformHelper::TryNormalize(swingDirection);
	}
	if (!hasSwingDirection &&
		!BuildReferenceDirection(
			ecs,
			attacker,
			attackerAbility,
			attackerTransform,
			AbilityCombatReferenceFrame::LockedActionDirection,
			swingDirection))
	{
		swingDirection = contactNormal;
	}

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

	outRecord = PendingCombatInteractionRecord{
		.sourceEntity = attacker,
		.sourceAbilityId = attackerAbility.abilityId,
		.sourceAbilityInstanceId = attackerAbility.abilityInstanceId,
		.sourceAttackWindowIndex = attackWindowIndex,
		.sourceColliderIndex = bestSourceColliderIndex,
		.targetColliderIndex = bestTargetColliderIndex,
		.resultType = bestResultType,
		.attackEffect = attackEffect,
		.guardEffect = guardEffect,
		.parryEffect = parryEffect,
		.maxKnockbackDistance = attackEffect.knockbackDistance,
		.maxHitStopSec = maxHitStopSec,
		.impactPoint = impactPoint,
		.swingDirection = swingDirection
	};
	return true;
}
