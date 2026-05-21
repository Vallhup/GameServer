#include "pch.h"
#include "BaseCombatantAspect.h"

#include "../ECS/GameplayRuntimeComponents.h"
#include "WorldRuntime.h"

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
	runtime.RegisterStorage<StaminaRecoveryStateComp>();
	runtime.RegisterStorage<GameplayTagStateComp>();
	runtime.RegisterStorage<GameplayEffectStateComp>();
	runtime.RegisterStorage<PendingProjectileSpawnComp>();
	runtime.RegisterStorage<PendingAbilityPresentationEventComp>();

	// 전투 시그널 (CommitCombatResultSystem 등이 런타임에 동적 부착).
	// Combatant feature 가 있는 캐릭터만 interrupt/버프 이벤트의 대상이 된다.
	runtime.RegisterStorage<AbilityInterruptQueueComp>();
	runtime.RegisterStorage<PendingGameplayEffectApplyComp>();
	runtime.RegisterStorage<PendingGameplayEffectRemoveComp>();

	// 킬 버프 수신 슬롯. EffectUser feature 를 가진 캐릭터(플레이어)에게만 부착된다.
	runtime.RegisterStorage<PendingKillBuffGrantComp>();
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
	runtime.DeferredUpsertComponent<CombatStatStateComp>(entity, 
		BuildCombatStatState(def, params.combatStatsOverride));
	runtime.DeferredAddComponent<StaminaRecoveryStateComp>(entity);
	runtime.DeferredAddComponent<GameplayTagStateComp>(entity);
	runtime.DeferredAddComponent<GameplayEffectStateComp>(entity);
	runtime.DeferredAddComponent<AbilityInterruptQueueComp>(entity);
	runtime.DeferredAddComponent<PendingGameplayEffectApplyComp>(entity);
	runtime.DeferredAddComponent<PendingGameplayEffectRemoveComp>(entity);
	runtime.DeferredAddComponent<PendingProjectileSpawnComp>(entity);
	runtime.DeferredAddComponent<PendingAbilityPresentationEventComp>(entity);

	if (def.HasFeature(CharacterFeatureFlags::EffectUser))
	{
		runtime.DeferredAddComponent<PendingKillBuffGrantComp>(entity);
	}
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

CombatStatStateComp BaseCombatantAspect::BuildCombatStatState(
	const CharacterDef& def, 
	const std::optional<CombatStatInitialState>& overrideStats) noexcept
{
	CombatStatStateComp stats{};

	if (overrideStats.has_value())
	{
		CombatStatInitialState initState = overrideStats.value();

		stats.currentHp		 = initState.currentHp;
		stats.maxHp			 = initState.maxHp;
		stats.currentStamina = initState.currentStamina;
		stats.maxStamina	 = initState.maxStamina;
		stats.currentPoise	 = initState.currentPoise;
		stats.maxPoise		 = initState.maxPoise;
		stats.attackPower	 = initState.attackPower;
		stats.defense		 = initState.defense;
		stats.attackSpeed	 = initState.attackSpeed;
		stats.moveSpeed		 = initState.moveSpeed;
	}

	else
	{
		stats.currentHp		 = static_cast<int32_t>(def.stat.maxHp);
		stats.maxHp			 = static_cast<int32_t>(def.stat.maxHp);
		stats.currentStamina = static_cast<int32_t>(def.stat.maxStamina);
		stats.maxStamina	 = static_cast<int32_t>(def.stat.maxStamina);
		stats.currentPoise	 = static_cast<int32_t>(def.stat.maxPoise);
		stats.maxPoise		 = static_cast<int32_t>(def.stat.maxPoise);
		stats.attackPower	 = static_cast<int32_t>(def.stat.attackPower);
		stats.defense		 = static_cast<int32_t>(def.stat.defense);
		stats.attackSpeed	 = def.stat.attackSpeed;
		stats.moveSpeed		 = def.stat.moveSpeed;
	}

	return stats;
}
