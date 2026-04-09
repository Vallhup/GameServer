#pragma once

#include <algorithm>
#include <cmath>
#include <optional>
#include <span>

#include "../../CharacterDef.h"
#include "WorldRuntime.h"
#include "../GameplayRuntimeComponents.h"

namespace GameplaySystemUtil
{
	inline constexpr float kWalkSpeedScale = 0.5f;
	inline constexpr float kYawTurnSpeedRad = 12.0f;
	inline constexpr float kOverlapEpsilon = 1.0e-4f;
	inline constexpr float kDefaultColliderRadius = 0.25f;
	inline constexpr float kPi = 3.14159265358979323846f;

	inline constexpr std::span<const AccessSpec> kNoAccesses{};
	inline constexpr std::span<const SystemTag> kNoDeps{};

	template<typename T>
	SystemMeta MakeSystemMeta(const char* name)
	{
		return SystemMeta{
			SystemTag(typeid(T)),
			name,
			kNoAccesses,
			kNoDeps,
			kNoDeps
		};
	}

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

	inline bool IsActionActive(const ActionStateComp& actionState)
	{
		return actionState.actionId != ActionId::None;
	}

	inline bool HasBlockingPendingState(const ECSView& ecs, Entity entity)
	{
		return
			ecs.HasComponent<PendingDespawnTag>(entity) ||
			ecs.HasComponent<PendingWorldTransferTag>(entity);
	}

	inline const CharacterActionProfileDef* FindCharacterActionProfile(
		CharacterId characterId)
	{
		const CharacterDef* characterDef = FindCharacterDef(characterId);
		if (characterDef == nullptr || !characterDef->action.has_value())
		{
			return nullptr;
		}

		return FindCharacterActionProfileDef(
			characterDef->action->actionProfileId);
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

		const CharacterDef* def = FindCharacterDef(spawnType->characterId);
		return (def != nullptr) ? &def->stat : nullptr;
	}

	inline AnimationId ResolveActionAnimationId(
		CharacterId characterId,
		ActionId actionId)
	{
		const CharacterActionProfileDef* profile =
			FindCharacterActionProfile(characterId);
		if (profile == nullptr)
		{
			return AnimationId::None;
		}

		const AnimationBindingProfileDef* bindingProfile =
			FindAnimationBindingProfileDef(profile->animationBindingProfileId);
		if (bindingProfile == nullptr)
		{
			return AnimationId::None;
		}

		for (const ActionAnimationBindingDef& binding : bindingProfile->actionBindings)
		{
			if (binding.actionId == actionId)
			{
				return binding.animationId;
			}
		}

		return AnimationId::None;
	}

	inline AnimationId ResolveLocomotionAnimationId(
		CharacterId characterId,
		LocomotionMode mode)
	{
		const CharacterActionProfileDef* profile =
			FindCharacterActionProfile(characterId);
		if (profile == nullptr)
		{
			return AnimationId::None;
		}

		const AnimationBindingProfileDef* bindingProfile =
			FindAnimationBindingProfileDef(profile->animationBindingProfileId);
		if (bindingProfile == nullptr)
		{
			return AnimationId::None;
		}

		for (const LocomotionAnimationBindingDef& binding :
			bindingProfile->locomotionBindings)
		{
			if (binding.mode == mode)
			{
				return binding.animationId;
			}
		}

		return AnimationId::None;
	}

	inline bool IsWindowActive(
		const ActionDef& actionDef,
		CombatWindowType type,
		float normalizedTime)
	{
		for (const ActionCombatWindowDef& window : actionDef.combatWindows)
		{
			if (window.windowType != type)
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

	inline std::optional<AttackCombatEffectDef> FindCurrentAttackEffect(
		const ActionDef& actionDef,
		float normalizedTime,
		uint16_t& outWindowIndex)
	{
		for (uint16_t i = 0;
			i < static_cast<uint16_t>(actionDef.combatWindows.size());
			++i)
		{
			const ActionCombatWindowDef& window = actionDef.combatWindows[i];
			if (window.windowType != CombatWindowType::Attack ||
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

		const CharacterDef* lhsDef = FindCharacterDef(lhsSpawn->characterId);
		const CharacterDef* rhsDef = FindCharacterDef(rhsSpawn->characterId);
		if (lhsDef == nullptr || rhsDef == nullptr)
		{
			return false;
		}

		return lhsDef->profile.faction == rhsDef->profile.faction;
	}

	inline void ClearActionTimelineAdvance(ActionTimelineAdvanceComp& advance)
	{
		advance.actionId = ActionId::None;
		advance.actionInstanceId = 0;
		advance.prevElapsedSec = 0.0f;
		advance.currElapsedSec = 0.0f;
		advance.events.clear();
	}
}
