#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <span>

#include "SystemMetaHelper.h"
#include "../../CharacterDef.h"
#include "../../DefEnumString.h"
#include "../../GameDataCatalog.h"
#include "../../GameplayContentCatalog.h"
#include "WorldRuntime.h"
#include "../GameplayRuntimeComponents.h"

namespace GameplaySystemUtil
{
	inline constexpr float kWalkSpeedScale = 0.5f;
	inline constexpr float kYawTurnSpeedRad = 12.0f;
	inline constexpr float kOverlapEpsilon = 1.0e-4f;
	inline constexpr float kDefaultColliderRadius = 0.25f;
	inline constexpr float kPi = 3.14159265358979323846f;

	inline constexpr std::span<const SystemTag> kNoDeps{};

	inline float ClampFloat(float value, float minValue, float maxValue)
	{
		return std::max(minValue, std::min(maxValue, value));
	}

	inline float LengthXZ(float x, float z)
	{
		return std::sqrt(x * x + z * z);
	}

	inline void NormalizeXZ(float& x, float& z)
	{
		const float length = LengthXZ(x, z);
		if (length <= kOverlapEpsilon)
		{
			x = 0.0f;
			z = 0.0f;
			return;
		}

		x /= length;
		z /= length;
	}

	inline float WrapYaw(float yawRad)
	{
		constexpr float twoPi = 2.0f * kPi;
		while (yawRad > kPi)
		{
			yawRad -= twoPi;
		}
		while (yawRad < -kPi)
		{
			yawRad += twoPi;
		}
		return yawRad;
	}

	inline float DirToYaw(float x, float z, float fallbackYaw)
	{
		if (LengthXZ(x, z) <= kOverlapEpsilon)
		{
			return fallbackYaw;
		}

		return std::atan2(-x, -z);
	}

	inline float ClampYawStep(float currentYaw, float targetYaw, float maxStep)
	{
		const float delta = WrapYaw(targetYaw - currentYaw);
		if (std::abs(delta) <= maxStep)
		{
			return targetYaw;
		}

		return WrapYaw(currentYaw + std::copysign(maxStep, delta));
	}

	inline bool IsAbilityActive(const AbilityStateComp& abilityState)
	{
		return abilityState.abilityId != InvalidAbilityId;
	}

	inline bool HasBlockingPendingState(const ECSView& ecs, Entity entity)
	{
		return
			ecs.HasComponent<PendingDespawnTag>(entity) ||
			ecs.HasComponent<PendingWorldTransferTag>(entity);
	}

	inline const AbilitySetDef* FindCharacterAbilitySet(
		CharacterId characterId)
	{
		const GameDataCatalog& gameDataCatalog = GameDataCatalog::Current();
		const GameplayContentCatalogSnapshot& contentCatalog =
			GameplayContentCatalogSnapshot::Current();
		const CharacterDef* characterDef =
			gameDataCatalog.Characters().Find(characterId);
		if (characterDef == nullptr ||
			!characterDef->abilitySetKey.has_value())
		{
			return nullptr;
		}

		return contentCatalog.FindAbilitySetByKey(*characterDef->abilitySetKey);
	}

	inline const CharacterStatDef* FindCharacterStats(
		const ECSView& ecs,
		Entity entity)
	{
		const SpawnTypeComp* spawnType = ecs.GetComponent<SpawnTypeComp>(entity);
		if (spawnType == nullptr)
		{
			return nullptr;
		}

		const CharacterDef* def =
			GameDataCatalog::Current().Characters().Find(spawnType->characterId);
		return (def != nullptr) ? &def->stat : nullptr;
	}

	inline AnimationId ResolveAbilityAnimationId(
		CharacterId characterId,
		AbilityId abilityId)
	{
		const AbilitySetDef* set = FindCharacterAbilitySet(characterId);
		if (set == nullptr || !set->animationBindingProfileId.has_value())
		{
			return AnimationId::None;
		}

		const AbilityAnimationBindingProfileDef* bindingProfile =
			GameplayContentCatalogSnapshot::Current()
				.AbilitySets()
				.animationBindingProfiles
				.Find(*set->animationBindingProfileId);
		if (bindingProfile == nullptr)
		{
			return AnimationId::None;
		}

		for (const AbilityAnimationBindingDef& binding :
			bindingProfile->abilityBindings)
		{
			if (binding.abilityId == abilityId)
			{
				AnimationId animationId{ AnimationId::None };
				return ParseAnimationIdString(binding.animationKey, animationId)
					? animationId
					: AnimationId::None;
			}
		}

		return AnimationId::None;
	}

	inline AnimationId ResolveLocomotionAnimationId(
		CharacterId characterId,
		LocomotionMode mode)
	{
		const AbilitySetDef* set = FindCharacterAbilitySet(characterId);
		if (set == nullptr || !set->animationBindingProfileId.has_value())
		{
			return AnimationId::None;
		}

		const AbilityAnimationBindingProfileDef* bindingProfile =
			GameplayContentCatalogSnapshot::Current()
				.AbilitySets()
				.animationBindingProfiles
				.Find(*set->animationBindingProfileId);
		if (bindingProfile == nullptr)
		{
			return AnimationId::None;
		}

		const auto FindBinding =
			[bindingProfile](LocomotionMode targetMode) noexcept
			{
				for (const AbilityLocomotionAnimationBindingDef& binding :
					bindingProfile->locomotionBindings)
				{
					if (binding.mode == targetMode)
					{
						return &binding;
					}
				}

				return static_cast<const AbilityLocomotionAnimationBindingDef*>(
					nullptr);
			};

		const AbilityLocomotionAnimationBindingDef* binding = FindBinding(mode);
		if (binding == nullptr)
		{
			switch (mode)
			{
			case LocomotionMode::WalkBack:
			case LocomotionMode::WalkLeft:
			case LocomotionMode::WalkRight:
				binding = FindBinding(LocomotionMode::Walk);
				break;
			case LocomotionMode::TurnLeft:
			case LocomotionMode::TurnRight:
				binding = FindBinding(LocomotionMode::Turn);
				break;
			default:
				break;
			}
		}

		if (binding == nullptr)
		{
			return AnimationId::None;
		}

		AnimationId animationId{ AnimationId::None };
		return ParseAnimationIdString(binding->animationKey, animationId)
			? animationId
			: AnimationId::None;
	}

	inline bool IsWindowActive(
		const AbilityDef& abilityDef,
		AbilityCombatWindowKind type,
		float normalizedTime)
	{
		for (const AbilityCombatWindowDef& window : abilityDef.timeline.combatWindows)
		{
			if (window.kind != type)
			{
				continue;
			}

			if (normalizedTime >= window.startNormalized &&
				normalizedTime < window.endNormalized)
			{
				return true;
			}
		}

		return false;
	}

	inline std::optional<AbilityAttackHitDef> FindCurrentAttackEffect(
		const AbilityDef& abilityDef,
		float normalizedTime,
		uint16_t& outWindowIndex)
	{
		for (uint16_t i = 0;
			i < static_cast<uint16_t>(abilityDef.timeline.combatWindows.size());
			++i)
		{
			const AbilityCombatWindowDef& window = abilityDef.timeline.combatWindows[i];
			if (window.kind != AbilityCombatWindowKind::Attack ||
				normalizedTime < window.startNormalized ||
				normalizedTime >= window.endNormalized ||
				!window.effect.has_value() ||
				!window.effect->attackHit.has_value())
			{
				continue;
			}

			outWindowIndex = i;
			return window.effect->attackHit;
		}

		return std::nullopt;
	}

	inline bool IsSameFaction(const ECSView& ecs, Entity lhs, Entity rhs)
	{
		const SpawnTypeComp* lhsSpawn = ecs.GetComponent<SpawnTypeComp>(lhs);
		const SpawnTypeComp* rhsSpawn = ecs.GetComponent<SpawnTypeComp>(rhs);
		if (lhsSpawn == nullptr || rhsSpawn == nullptr)
		{
			return false;
		}

		const GameDataCatalog& catalog = GameDataCatalog::Current();
		const CharacterDef* lhsDef =
			catalog.Characters().Find(lhsSpawn->characterId);
		const CharacterDef* rhsDef =
			catalog.Characters().Find(rhsSpawn->characterId);
		if (lhsDef == nullptr || rhsDef == nullptr)
		{
			return false;
		}

		return lhsDef->profile.faction == rhsDef->profile.faction;
	}

	inline void ClearAbilityTimelineAdvance(AbilityTimelineAdvanceComp& advance)
	{
		advance.abilityId = InvalidAbilityId;
		advance.abilityInstanceId = 0;
		advance.prevElapsedSec = 0.0f;
		advance.currElapsedSec = 0.0f;
		advance.startedThisFrame = false;
		advance.events.clear();
	}
}
