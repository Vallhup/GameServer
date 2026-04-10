#include "pch.h"
#include "ResolveCombatHitSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;
using namespace DirectX;

const SystemMeta ResolveCombatHitSystem::kMeta =
	MakeSystemMeta<ResolveCombatHitSystem>("ResolveCombatHitSystem");

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
		auto* attackerAction =
			ctx.ecs.GetMutableComponent<ActionStateComp>(attacker);
		auto* attackerTransform =
			ctx.ecs.GetMutableComponent<WorldTransformComp>(attacker);
		auto* attackerColliders =
			ctx.ecs.GetMutableComponent<SkeletalCombatColliderComp>(attacker);
		auto* attackerDedup =
			ctx.ecs.GetMutableComponent<CombatHitDedupStateComp>(attacker);
		if (attackerAction == nullptr || attackerTransform == nullptr ||
			attackerColliders == nullptr)
		{
			continue;
		}

		if (attackerDedup != nullptr &&
			attackerDedup->boundActionInstanceId !=
				attackerAction->actionInstanceId)
		{
			attackerDedup->boundActionInstanceId =
				attackerAction->actionInstanceId;
			attackerDedup->resolvedVictims.clear();
		}

		const ActionDef* actionDef = FindActionDef(attackerAction->actionId);
		if (actionDef == nullptr || actionDef->duration <= 0.0f)
		{
			continue;
		}

		uint16_t windowIndex = 0;
		const float normalizedTime = ClampFloat(
			attackerAction->elapsedSec / actionDef->duration,
			0.0f,
			1.0f);
		const std::optional<AttackCombatEffectDef> attackEffect =
			FindCurrentAttackEffect(*actionDef, normalizedTime, windowIndex);
		if (!attackEffect.has_value())
		{
			continue;
		}

		for (auto [victim, victimAction, victimTransform, victimColliders, victimActivation] :
			ctx.ecs.View<
				ActionStateComp,
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
				*attackerAction,
				*attackerTransform,
				*attackerColliders,
				*attackEffect,
				windowIndex,
				victim,
				victimAction,
				victimTransform,
				victimColliders,
				victimActivation,
				interaction))
			{
				continue;
			}

			victimResult->receivedInteractions.push_back(interaction);
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

XMMATRIX ResolveCombatHitSystem::BuildWorldMatrix(
	const WorldTransformComp& transform)
{
	return XMMatrixAffineTransformation(
		XMLoadFloat3(&transform.scale),
		XMVectorZero(),
		XMLoadFloat4(&transform.rotation),
		XMLoadFloat3(&transform.position));
}

float ResolveCombatHitSystem::BuildRadiusScale(
	const WorldTransformComp& transform) noexcept
{
	return std::max(
		transform.scale.x,
		std::max(transform.scale.y, transform.scale.z));
}

XMFLOAT3 ResolveCombatHitSystem::TransformPoint(
	const XMMATRIX& worldMatrix,
	const XMFLOAT3& point)
{
	XMFLOAT3 transformed{};
	XMStoreFloat3(
		&transformed,
		XMVector3TransformCoord(XMLoadFloat3(&point), worldMatrix));
	return transformed;
}

float ResolveCombatHitSystem::SegmentSegmentDistanceSq(
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

	const auto dot = [](const XMFLOAT3& a, const XMFLOAT3& b) noexcept
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	};

	const float a = dot(u, u);
	const float b = dot(u, v);
	const float c = dot(v, v);
	const float d = dot(u, w);
	const float e = dot(v, w);
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

	const XMFLOAT3 delta{
		w.x + s * u.x - t * v.x,
		w.y + s * u.y - t * v.y,
		w.z + s * u.z - t * v.z
	};
	return dot(delta, delta);
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
	const ActionStateComp& victimAction,
	CombatResolveResultType resultType,
	std::optional<GuardCombatEffectDef>& outGuardEffect,
	std::optional<ParryCombatEffectDef>& outParryEffect,
	float& outMaxHitStopSec)
{
	outGuardEffect.reset();
	outParryEffect.reset();
	outMaxHitStopSec = 0.0f;

	if (victimAction.actionId == ActionId::None)
	{
		return true;
	}

	const ActionDef* victimActionDef = FindActionDef(victimAction.actionId);
	if (victimActionDef == nullptr || victimActionDef->duration <= 0.0f)
	{
		return true;
	}

	const float normalizedTime = ClampFloat(
		victimAction.elapsedSec / victimActionDef->duration,
		0.0f,
		1.0f);

	for (const ActionCombatWindowDef& window : victimActionDef->combatWindows)
	{
		if (normalizedTime < window.startNormalized ||
			normalizedTime >= window.endNormalized ||
			!window.effect.has_value())
		{
			continue;
		}

		if (resultType == CombatResolveResultType::Guard &&
			window.windowType == CombatWindowType::Guard &&
			window.effect->guardResponse.has_value())
		{
			outGuardEffect = window.effect->guardResponse;
			outMaxHitStopSec = outGuardEffect->hitStopSec;
			return true;
		}

		if (resultType == CombatResolveResultType::Parry &&
			window.windowType == CombatWindowType::Parry &&
			window.effect->parryResponse.has_value())
		{
			outParryEffect = window.effect->parryResponse;
			outMaxHitStopSec = outParryEffect->hitStopSec;
			return true;
		}
	}

	return true;
}

const ActionCombatWindowDef* ResolveCombatHitSystem::FindActiveCombatWindow(
	const ActionStateComp& actionState,
	CombatWindowType windowType)
{
	if (actionState.actionId == ActionId::None)
	{
		return nullptr;
	}

	const ActionDef* actionDef = FindActionDef(actionState.actionId);
	if (actionDef == nullptr || actionDef->duration <= 0.0f)
	{
		return nullptr;
	}

	const float normalizedTime = ClampFloat(
		actionState.elapsedSec / actionDef->duration,
		0.0f,
		1.0f);

	for (const ActionCombatWindowDef& window : actionDef->combatWindows)
	{
		if (window.windowType != windowType ||
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
	const ActionCombatWindowDef& window,
	const AttackCombatEffectDef& attackEffect) noexcept
{
	if (!window.appliesTo.has_value())
	{
		return true;
	}

	switch (*window.appliesTo)
	{
	case ActionCombatApplyTo::ParryableAttack:
		return attackEffect.parryable;
	case ActionCombatApplyTo::GuardableAttack:
		return attackEffect.guardable;
	case ActionCombatApplyTo::FrontPhysical:
	default:
		return true;
	}
}

bool ResolveCombatHitSystem::BuildReferenceDirection(
	const ECSView& ecs,
	Entity owner,
	const ActionStateComp& ownerAction,
	const WorldTransformComp& ownerTransform,
	CombatReferenceFrame referenceFrame,
	XMFLOAT3& outDirection) noexcept
{
	auto normalizeXZ =
		[](float& x, float& z) noexcept -> bool
		{
			const float lengthSq = x * x + z * z;
			if (lengthSq <= 1.0e-6f)
			{
				x = 0.0f;
				z = 0.0f;
				return false;
			}

			const float invLength = 1.0f / std::sqrt(lengthSq);
			x *= invLength;
			z *= invLength;
			return true;
		};

	float dirX = 0.0f;
	float dirZ = 0.0f;
	switch (referenceFrame)
	{
	case CombatReferenceFrame::MoveDirection:
		if (const auto* locomotion =
			ecs.GetComponent<LocomotionStateComp>(owner))
		{
			dirX = locomotion->desiredMoveDirX;
			dirZ = locomotion->desiredMoveDirZ;
		}
		break;
	case CombatReferenceFrame::LockedActionDirection:
		dirX = ownerAction.directionX;
		dirZ = ownerAction.directionZ;
		break;
	case CombatReferenceFrame::OwnerFacing:
	default:
		break;
	}

	if (!normalizeXZ(dirX, dirZ))
	{
		const XMVECTOR facing =
			XMVector3TransformNormal(
				XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f),
				BuildWorldMatrix(ownerTransform));
		dirX = XMVectorGetX(facing);
		dirZ = XMVectorGetZ(facing);
		if (!normalizeXZ(dirX, dirZ))
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
	const ActionStateComp& sourceAction,
	const WorldTransformComp& sourceTransform,
	const WorldTransformComp& targetTransform,
	const ActionCombatSpatialFilterDef& spatialFilter) noexcept
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
	const float lengthSq = toAttackerX * toAttackerX + toAttackerZ * toAttackerZ;
	if (lengthSq <= 1.0e-6f)
	{
		return false;
	}

	const float invLength = 1.0f / std::sqrt(lengthSq);
	toAttackerX *= invLength;
	toAttackerZ *= invLength;

	XMFLOAT3 referenceDirection{};
	if (!BuildReferenceDirection(
		ecs,
		source,
		sourceAction,
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
	const ActionStateComp& attackerAction,
	const WorldTransformComp& attackerTransform,
	const SkeletalCombatColliderComp& attackerColliders,
	const AttackCombatEffectDef& attackEffect,
	uint16_t attackWindowIndex,
	Entity victim,
	const ActionStateComp& victimAction,
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

	const ActionDef* attackerActionDef = FindActionDef(attackerAction.actionId);
	if (attackerActionDef == nullptr ||
		attackWindowIndex >= attackerActionDef->combatWindows.size())
	{
		return false;
	}

	const ActionCombatWindowDef& attackWindow =
		attackerActionDef->combatWindows[attackWindowIndex];
	if (attackWindow.windowType != CombatWindowType::Attack)
	{
		return false;
	}

	if (attackWindow.spatialFilter.has_value() &&
		!PassesSpatialFilter(
			ecs,
			attacker,
			attackerAction,
			attackerTransform,
			victimTransform,
			*attackWindow.spatialFilter))
	{
		return false;
	}

	const XMMATRIX attackerWorldMatrix = BuildWorldMatrix(attackerTransform);
	const XMMATRIX victimWorldMatrix = BuildWorldMatrix(victimTransform);
	const float attackerRadiusScale = BuildRadiusScale(attackerTransform);
	const float victimRadiusScale = BuildRadiusScale(victimTransform);

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
			TransformPoint(attackerWorldMatrix, sourceCollider.capsule.p0),
			TransformPoint(attackerWorldMatrix, sourceCollider.capsule.p1)
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
				const ActionCombatWindowDef* parryWindow =
					FindActiveCombatWindow(victimAction, CombatWindowType::Parry);
				if (parryWindow != nullptr &&
					DoesWindowApplyToAttack(*parryWindow, attackEffect) &&
					(!parryWindow->spatialFilter.has_value() ||
						PassesSpatialFilter(
							ecs,
							victim,
							victimAction,
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
				const ActionCombatWindowDef* guardWindow =
					FindActiveCombatWindow(victimAction, CombatWindowType::Guard);
				if (guardWindow != nullptr &&
					DoesWindowApplyToAttack(*guardWindow, attackEffect) &&
					(!guardWindow->spatialFilter.has_value() ||
						PassesSpatialFilter(
							ecs,
							victim,
							victimAction,
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
				TransformPoint(victimWorldMatrix, targetCollider.capsule.p0),
				TransformPoint(victimWorldMatrix, targetCollider.capsule.p1)
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

	std::optional<GuardCombatEffectDef> guardEffect;
	std::optional<ParryCombatEffectDef> parryEffect;
	float maxHitStopSec = attackEffect.hitStopSec;
	TryResolveDefensiveEffects(
		victimAction,
		bestResultType,
		guardEffect,
		parryEffect,
		maxHitStopSec);
	maxHitStopSec = std::max(maxHitStopSec, attackEffect.hitStopSec);

	outRecord = PendingCombatInteractionRecord{
		.sourceEntity = attacker,
		.sourceActionId = attackerAction.actionId,
		.sourceActionInstanceId = attackerAction.actionInstanceId,
		.sourceAttackWindowIndex = attackWindowIndex,
		.sourceColliderIndex = bestSourceColliderIndex,
		.targetColliderIndex = bestTargetColliderIndex,
		.resultType = bestResultType,
		.attackEffect = attackEffect,
		.guardEffect = guardEffect,
		.parryEffect = parryEffect,
		.maxKnockbackDistance = attackEffect.knockbackDistance,
		.maxHitStopSec = maxHitStopSec
	};
	return true;
}

const SystemMeta& ResolveCombatHitSystem::Meta() const
{
	return kMeta;
}
