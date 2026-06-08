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
#include "../../TransformHelper.h"

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
		return std::clamp(value, minValue, maxValue);
	}

	// 아래 수학 함수들은 TransformHelper에 정의된 구현을 사용합니다.
	// 기존 호출자의 'using namespace GameplaySystemUtil' 호환성을 유지합니다.

	inline float LengthXZ(float x, float z)
	{
		return TransformHelper::LengthXZ(x, z);
	}

	inline void NormalizeXZ(float& x, float& z)
	{
		TransformHelper::NormalizeXZ(x, z);
	}

	inline float WrapYaw(float yawRad)
	{
		return TransformHelper::WrapPi(yawRad);
	}

	inline float DirToYaw(float x, float z, float fallbackYaw)
	{
		return TransformHelper::DirToYaw(x, z, fallbackYaw);
	}

	inline float ClampYawStep(float currentYaw, float targetYaw, float maxStep)
	{
		return TransformHelper::ClampYawStep(currentYaw, targetYaw, maxStep);
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

	inline GameplayTagMask GameplayTagBit(GameplayTagId tagId)
	{
		if (tagId == InvalidGameplayTagId || tagId > 64)
		{
			return 0;
		}

		return GameplayTagMask{ 1 } << (tagId - 1);
	}

	inline GameplayTagMask BuildGameplayTagMask(
		const GameplayTagStateComp* tagState)
	{
		if (tagState == nullptr)
		{
			return 0;
		}

		GameplayTagMask mask = tagState->dynamicTags;
		for (const GameplayTagCountEntry& entry : tagState->countedTags)
		{
			if (entry.count > 0)
			{
				mask |= GameplayTagBit(entry.tagId);
			}
		}

		return mask;
	}

	inline bool HasGameplayTag(GameplayTagMask mask, GameplayTagId tagId)
	{
		const GameplayTagMask bit = GameplayTagBit(tagId);
		return bit != 0 && (mask & bit) != 0;
	}

	inline bool EvaluateGameplayTagQuery(
		const GameplayTagQueryDef& query,
		GameplayTagMask mask,
		bool noneResult)
	{
		switch (query.op)
		{
		case GameplayTagQueryOp::None:
			return noneResult;
		case GameplayTagQueryOp::All:
			for (GameplayTagId tagId : query.tags)
			{
				if (!HasGameplayTag(mask, tagId))
				{
					return false;
				}
			}
			for (const GameplayTagQueryDef& child : query.children)
			{
				if (!EvaluateGameplayTagQuery(child, mask, noneResult))
				{
					return false;
				}
			}
			return true;
		case GameplayTagQueryOp::Any:
			for (GameplayTagId tagId : query.tags)
			{
				if (HasGameplayTag(mask, tagId))
				{
					return true;
				}
			}
			for (const GameplayTagQueryDef& child : query.children)
			{
				if (EvaluateGameplayTagQuery(child, mask, noneResult))
				{
					return true;
				}
			}
			return false;
		case GameplayTagQueryOp::Not:
			for (GameplayTagId tagId : query.tags)
			{
				if (HasGameplayTag(mask, tagId))
				{
					return false;
				}
			}
			for (const GameplayTagQueryDef& child : query.children)
			{
				if (EvaluateGameplayTagQuery(child, mask, noneResult))
				{
					return false;
				}
			}
			return true;
		default:
			return noneResult;
		}
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

	inline bool AllowsSameFactionCombat(const WorldRuntime& runtime) noexcept
	{
		const WorldDef* const worldDef = runtime.GetDef();
		return worldDef != nullptr && worldDef->id == WorldDefId::Pvp;
	}

	inline bool ShouldBlockSameFactionCombat(
		const WorldRuntime& runtime,
		const ECSView& ecs,
		Entity lhs,
		Entity rhs)
	{
		return
			!AllowsSameFactionCombat(runtime) &&
			IsSameFaction(ecs, lhs, rhs);
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
