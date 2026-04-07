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

	inline PlayerActionInput ToActionInput(PlayerActionInputType type)
	{
		switch (type) {
		case PlayerActionInputType::LightAttack:
			return PlayerActionInput::LightAttack;
		case PlayerActionInputType::HeavyAttack:
			return PlayerActionInput::HeavAttack;
		case PlayerActionInputType::Dodge:
			return PlayerActionInput::Dodge;
		case PlayerActionInputType::Parry:
			return PlayerActionInput::Parry;
		case PlayerActionInputType::None:
		default:
			return PlayerActionInput::None;
		}
	}

	inline ActionKind ToActionKind(PlayerActionInputType type)
	{
		switch (type) {
		case PlayerActionInputType::LightAttack:
		case PlayerActionInputType::HeavyAttack:
			return ActionKind::Attack;
		case PlayerActionInputType::Dodge:
			return ActionKind::Dodge;
		case PlayerActionInputType::Parry:
			return ActionKind::Parry;
		case PlayerActionInputType::None:
		default:
			return ActionKind::None;
		}
	}

	inline ActionId FindActionForInput(
		CharacterId characterId,
		PlayerActionInputType inputType)
	{
		const PlayerActionInput expectedInput = ToActionInput(inputType);
		const ActionKind expectedKind = ToActionKind(inputType);

		for (const ActionDef& def : GetActionDefs())
		{
			if (def.characterId != characterId)
			{
				continue;
			}

			if (inputType == PlayerActionInputType::Dodge)
			{
				if (def.kind == ActionKind::Dodge)
				{
					return def.id;
				}
				continue;
			}

			if (def.playerInput == expectedInput && def.kind == expectedKind)
			{
				return def.id;
			}
		}

		return ActionId::None;
	}

	inline ActionId FindReactionAction(
		CharacterId characterId,
		CombatReactionKind reactionKind)
	{
		const ActionKind expectedKind =
			(reactionKind == CombatReactionKind::HitReaction)
			? ActionKind::Hit
			: ActionKind::Stun;

		for (const ActionDef& def : GetActionDefs())
		{
			if (def.characterId == characterId && def.kind == expectedKind)
			{
				return def.id;
			}
		}

		return ActionId::None;
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

	inline AnimationId ResolveActionAnimationId(ActionId actionId)
	{
		switch (actionId) {
		case ActionId::Knight_LightAttack1:
			return AnimationId::Knight_LightAttack1;
		/*case ActionId::Knight_LightAttack2:
			return AnimationId::Knight_LightAttack2;
		case ActionId::Knight_LightAttack3:
			return AnimationId::Knight_LightAttack3;
		case ActionId::Knight_HeavyAttack:
			return AnimationId::Knight_HeavyAttack;
		case ActionId::Knight_SpecialAttack:
			return AnimationId::Knight_SpecialAttack;*/
		case ActionId::Knight_Dodge:
			return AnimationId::Knight_Dodge;
		case ActionId::Knight_Parry:
			return AnimationId::Knight_Parry;
		case ActionId::Knight_Stun:
			return AnimationId::Knight_Stun;
		case ActionId::Knight_Hit:
			return AnimationId::Knight_Hit;
		case ActionId::Knight_Guard:
			return AnimationId::Knight_Guard;
		case ActionId::Knight_UseHpPotion:
			return AnimationId::Knight_Drinking;
		case ActionId::Knight_Dead:
			return AnimationId::Knight_Death;

		/*case ActionId::Imp_melee1:
			return AnimationId::Imp_Melee_1;
		case ActionId::Imp_melee2:
			return AnimationId::Imp_Melee_2;
		case ActionId::Imp_melee3:
			return AnimationId::Imp_Melee_3;
		case ActionId::Imp_melee4:
			return AnimationId::Imp_Melee_4;
		case ActionId::Imp_melee5:
			return AnimationId::Imp_Melee_5;
		case ActionId::Imp_Stun:
			return AnimationId::Imp_Stun;
		case ActionId::Imp_Hit:
			return AnimationId::Imp_React_Front;
		case ActionId::Imp_Dead:
			return AnimationId::Imp_Death_1;*/

		case ActionId::FinalBoss_Thrust:
			return AnimationId::FinalBoss_Thrust;
		case ActionId::FinalBoss_Slash:
			return AnimationId::FinalBoss_Slash;
		case ActionId::FinalBoss_DashSlash:
			return AnimationId::FinalBoss_DashSlash;
		case ActionId::FinalBoss_JumpSlash:
			return AnimationId::FinalBoss_JumpSlash;
		case ActionId::FinalBoss_MultiSlash:
			return AnimationId::FinalBoss_MultiSlash;
		case ActionId::FinalBoss_Stun:
			return AnimationId::FinalBoss_Stun;
		case ActionId::FinalBoss_Hit:
			return AnimationId::FinalBoss_Hit;
		case ActionId::FinalBoss_Dead:
			return AnimationId::FinalBoss_Death;

		default:
			return AnimationId::None;
		}
	}

	inline AnimationId ResolveLocomotionAnimationId(
		CharacterId characterId,
		LocomotionMode mode)
	{
		switch (characterId) {
		case CharacterId::Knight:
			switch (mode) {
			case LocomotionMode::Idle:
				return AnimationId::Knight_Idle;
			case LocomotionMode::Walk:
			case LocomotionMode::Turn:
				return AnimationId::Knight_Walk;
			case LocomotionMode::Run:
				return AnimationId::Knight_Run;
			default:
				return AnimationId::Knight_Idle;
			}

		/*case CharacterId::Imp:
			switch (mode) {
			case LocomotionMode::Idle:
				return AnimationId::Imp_Idle_1;
			case LocomotionMode::Walk:
			case LocomotionMode::Run:
			case LocomotionMode::Turn:
				return AnimationId::Imp_Walk_Forward;
			default:
				return AnimationId::Imp_Idle_1;
			}*/

		case CharacterId::FinalBoss:
			switch (mode) {
			case LocomotionMode::Idle:
				return AnimationId::FinalBoss_Idle;
			case LocomotionMode::Walk:
			case LocomotionMode::Run:
			case LocomotionMode::Turn:
				return AnimationId::FinalBoss_Walk;
			default:
				return AnimationId::FinalBoss_Idle;
			}

		default:
			return AnimationId::None;
		}
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

	template<typename T>
	T* MutableComponent(ECSView& ecs, Entity entity)
	{
		return const_cast<T*>(ecs.GetComponent<T>(entity));
	}
}
