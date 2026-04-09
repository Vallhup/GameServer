#pragma once

#include <DirectXMath.h>
#include <optional>

#include "../../GameplayRuntimeComponents.h"
#include "System.h"

class ResolveCombatHitSystem final : public System {
public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override;

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
		const ActionStateComp& victimAction,
		CombatResolveResultType resultType,
		std::optional<GuardCombatEffectDef>& outGuardEffect,
		std::optional<ParryCombatEffectDef>& outParryEffect,
		float& outMaxHitStopSec);
	static const ActionCombatWindowDef* FindActiveCombatWindow(
		const ActionStateComp& actionState,
		CombatWindowType windowType);
	static bool DoesWindowApplyToAttack(
		const ActionCombatWindowDef& window,
		const AttackCombatEffectDef& attackEffect) noexcept;
	static bool BuildReferenceDirection(
		const ECSView& ecs,
		Entity victim,
		const ActionStateComp& victimAction,
		const WorldTransformComp& victimTransform,
		CombatReferenceFrame referenceFrame,
		DirectX::XMFLOAT3& outDirection) noexcept;
	static bool PassesSpatialFilter(
		const ECSView& ecs,
		Entity attacker,
		const WorldTransformComp& attackerTransform,
		Entity victim,
		const ActionStateComp& victimAction,
		const WorldTransformComp& victimTransform,
		const ActionCombatSpatialFilterDef& spatialFilter) noexcept;
	static bool TryBuildInteractionRecord(
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
		PendingCombatInteractionRecord& outRecord);

	static const SystemMeta kMeta;
};
