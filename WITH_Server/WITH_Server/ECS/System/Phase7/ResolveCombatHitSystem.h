#pragma once

#include <DirectXMath.h>
#include <optional>

#include "../../../AbilityDef.h"

#include "System.h"
#include "SystemMetaStorage.h"

enum class SkeletalCombatColliderRoleMask : uint8_t;
enum class CombatResolveResultType : uint8_t;
enum class AbilityCombatWindowKind : uint8_t;

struct Capsule;

struct WorldTransformComp;
struct AbilityStateComp;
struct AbilityMoveDeltaComp;
struct AbilityMoveRuntimeComp;
struct SkeletalCombatColliderComp;
struct CombatColliderActivationComp;
struct PendingCombatInteractionRecord;

class ResolveCombatHitSystem final : public System {
	static const StaticSystemMetaStorage<11> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }

private:
	static bool HasRole(
		uint8_t roleMask,
		SkeletalCombatColliderRoleMask role) noexcept;

	static DirectX::XMMATRIX BuildWorldMatrix(
		const WorldTransformComp& transform);

	static float BuildRadiusScale(
		const WorldTransformComp& transform) noexcept;

	static DirectX::XMFLOAT3 TransformPoint(
		const DirectX::XMMATRIX& worldMatrix,
		const DirectX::XMFLOAT3& point);

	static float SegmentSegmentDistanceSq(
		const Capsule& lhs,
		const Capsule& rhs) noexcept;

	static bool CapsulesOverlap(
		const Capsule& lhs,
		float lhsRadius,
		const Capsule& rhs,
		float rhsRadius) noexcept;

	static int GetResultPriority(
		CombatResolveResultType resultType) noexcept;

	static bool TryResolveDefensiveEffects(
		const AbilityStateComp& victimAbility,
		CombatResolveResultType resultType,
		std::optional<AbilityGuardResponseDef>& outGuardEffect,
		std::optional<AbilityParryResponseDef>& outParryEffect,
		float& outMaxHitStopSec);

	static const AbilityCombatWindowDef* FindActiveCombatWindow(
		const AbilityStateComp& abilityState,
		AbilityCombatWindowKind windowType);

	static bool DoesWindowApplyToAttack(
		const AbilityCombatWindowDef& window,
		const AbilityAttackHitDef& attackEffect) noexcept;

	static bool BuildReferenceDirection(
		const ECSView& ecs,
		Entity owner,
		const AbilityStateComp& ownerAbility,
		const WorldTransformComp& ownerTransform,
		AbilityCombatReferenceFrame referenceFrame,
		DirectX::XMFLOAT3& outDirection) noexcept;

	static bool PassesSpatialFilter(
		const ECSView& ecs,
		Entity source,
		const AbilityStateComp& sourceAbility,
		const WorldTransformComp& sourceTransform,
		const WorldTransformComp& targetTransform,
		const AbilityCombatSpatialFilterDef& spatialFilter) noexcept;

	static bool TryBuildInteractionRecord(
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
		PendingCombatInteractionRecord& outRecord);
};
