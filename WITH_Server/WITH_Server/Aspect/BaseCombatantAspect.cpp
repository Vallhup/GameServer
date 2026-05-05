#include "pch.h"
#include "BaseCombatantAspect.h"

#include "../ECS/GameplayRuntimeComponents.h"
#include "WorldRuntime.h"

namespace
{
	CombatStatStateComp MakeInitialCombatStats(
		const CharacterDef& def) noexcept
	{
		CombatStatStateComp stats{};
		stats.currentHp		 = static_cast<int32_t>(def.stat.maxHp);
		stats.maxHp			 = static_cast<int32_t>(def.stat.maxHp);
		stats.currentStamina = static_cast<int32_t>(def.stat.maxStamina);
		stats.maxStamina     = static_cast<int32_t>(def.stat.maxStamina);
		stats.currentPoise   = static_cast<int32_t>(def.stat.maxPoise);
		stats.maxPoise       = static_cast<int32_t>(def.stat.maxPoise);
		stats.attackPower    = static_cast<int32_t>(def.stat.attackPower);
		stats.defense        = static_cast<int32_t>(def.stat.defense);
		stats.attackSpeed    = def.stat.attackSpeed;
		stats.moveSpeed      = def.stat.moveSpeed;
		return stats;
	}

	CombatStatStateComp ToCombatStatState(
		const CombatStatInitialState& initial) noexcept
	{
		CombatStatStateComp stats{};
		stats.currentHp = initial.currentHp;
		stats.maxHp = initial.maxHp;
		stats.currentStamina = initial.currentStamina;
		stats.maxStamina = initial.maxStamina;
		stats.currentPoise = initial.currentPoise;
		stats.maxPoise = initial.maxPoise;
		stats.attackPower = initial.attackPower;
		stats.defense = initial.defense;
		stats.attackSpeed = initial.attackSpeed;
		stats.moveSpeed = initial.moveSpeed;
		return stats;
	}
}

CharacterFeatureFlags BaseCombatantAspect::RequiredFeature() const noexcept
{
	return CharacterFeatureFlags::Combatant;
}

void BaseCombatantAspect::RegisterStorages(WorldRuntime& runtime) const
{
	runtime.RegisterStorage<ActorInputComp>();
	runtime.RegisterStorage<AbilityStateComp>();
	runtime.RegisterStorage<LocomotionStateComp>();
	runtime.RegisterStorage<AbilityTimelineAdvanceComp>();
	runtime.RegisterStorage<AnimationPlaybackStateComp>();
	runtime.RegisterStorage<SampledAnimationPoseComp>();
	runtime.RegisterStorage<SkeletalCombatColliderComp>();
	runtime.RegisterStorage<CombatStatStateComp>();
	runtime.RegisterStorage<GameplayEffectStateComp>();
	runtime.RegisterStorage<PendingProjectileSpawnComp>();
	runtime.RegisterStorage<PendingAbilityPresentationEventComp>();

	// 전투 시그널 (CommitCombatResultSystem 등이 런타임에 동적 부착).
	// Combatant feature 가 있는 캐릭터만 interrupt/버프 이벤트의 대상이 된다.
	runtime.RegisterStorage<AbilityInterruptQueueComp>();
	runtime.RegisterStorage<PendingGameplayEffectApplyComp>();
	runtime.RegisterStorage<PendingGameplayEffectRemoveComp>();
}

void BaseCombatantAspect::Attach(
	WorldRuntime& runtime,
	Entity entity,
	const CharacterDef& def,
	const AssembleParams& params) const
{
	runtime.DeferredAddComponent<ActorInputComp>(entity);
	runtime.DeferredAddComponent<AbilityStateComp>(entity);
	runtime.DeferredAddComponent<LocomotionStateComp>(entity);
	runtime.DeferredAddComponent<AbilityTimelineAdvanceComp>(entity);
	runtime.DeferredAddComponent<AnimationPlaybackStateComp>(entity);
	runtime.DeferredAddComponent<SampledAnimationPoseComp>(entity);
	runtime.DeferredAddComponent<SkeletalCombatColliderComp>(entity);
	runtime.DeferredUpsertComponent<CombatStatStateComp>(
		entity,
		params.combatStatsOverride.has_value()
			? ToCombatStatState(*params.combatStatsOverride)
			: MakeInitialCombatStats(def));
	runtime.DeferredAddComponent<GameplayEffectStateComp>(entity);
	runtime.DeferredAddComponent<AbilityInterruptQueueComp>(entity);
	runtime.DeferredAddComponent<PendingProjectileSpawnComp>(entity);
	runtime.DeferredAddComponent<PendingAbilityPresentationEventComp>(entity);
}

bool BaseCombatantAspect::Validate(
	const CharacterDef& def,
	std::string& outError) const
{
	if (def.stat.maxHp == 0)
	{
		outError = "Combatant requires stat.maxHp > 0";
		return false;
	}
	return true;
}
