#include "pch.h"
#include "RecoverStaminaSystem.h"

#include "../GameplaySystemUtil.h"
#include "../Phase2/ResolveAbilityStateSystem.h"
#include "CommitCombatResultSystem.h"

using namespace GameplaySystemUtil;

namespace
{
	float ResolveActiveAbilityRegenMultiplier(
		const AbilityStateComp& abilityState,
		const StaminaRecoveryTuning& tuning)
	{
		if (!IsAbilityActive(abilityState))
		{
			return 1.0f;
		}

		const AbilityDef* abilityDef =
			GameplayContentCatalogSnapshot::Current().Abilities().Find(
				abilityState.abilityId);
		if (abilityDef == nullptr)
		{
			return 0.0f;
		}

		switch (abilityDef->kind)
		{
		case AbilityKind::Attack:
		case AbilityKind::Dodge:
		case AbilityKind::Parry:
		case AbilityKind::Dead:
			return 0.0f;
		case AbilityKind::Guard:
			return std::max(0.0f, tuning.guardRegenMultiplier);
		case AbilityKind::Reaction:
		case AbilityKind::UseItem:
		case AbilityKind::NonCombat:
		case AbilityKind::None:
		default:
			return 1.0f;
		}
	}
}

const StaticSystemMetaStorage<6, 0, 2> RecoverStaminaSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<RecoverStaminaSystem>(),
		"RecoverStaminaSystem",
		std::array<AccessSpec, 6>
	{
		WriteImmediate(ComponentRes<CombatStatStateComp>()),
		WriteImmediate(ComponentRes<StaminaRecoveryStateComp>()),
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		WriteImmediate(ComponentRes<DirtyFlagsComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
	},
		std::array<SystemTag, 0>{},
		std::array<SystemTag, 2>
		{
			SysTag<ResolveAbilityStateSystem>(),
			SysTag<CommitCombatResultSystem>(),
		});

void RecoverStaminaSystem::Execute(SystemContext& ctx)
{
	const float dtSec =
		static_cast<float>(std::max(0.0, ctx.dtSec));
	if (dtSec <= 0.0f)
	{
		return;
	}

	for (auto [entity, stats, recovery, abilityState] :
		ctx.ecs.View<
			CombatStatStateComp,
			StaminaRecoveryStateComp,
			AbilityStateComp>())
	{
		if (HasBlockingPendingState(ctx.ecs, entity) ||
			stats.currentHp <= 0 ||
			stats.maxStamina <= 0)
		{
			recovery.regenRemainder = 0.0f;
			continue;
		}

		if (stats.currentStamina >= stats.maxStamina)
		{
			recovery.regenRemainder = 0.0f;
			continue;
		}

		const float activeAbilityMultiplier =
			ResolveActiveAbilityRegenMultiplier(abilityState, recovery.tuning);
		if (activeAbilityMultiplier <= 0.0f)
		{
			recovery.regenRemainder = 0.0f;
			continue;
		}

		if (recovery.regenLockRemainingSec > 0.0f)
		{
			recovery.regenLockRemainingSec =
				std::max(0.0f, recovery.regenLockRemainingSec - dtSec);
			recovery.regenRemainder = 0.0f;
			continue;
		}

		const float regenPerSec =
			std::max(0.0f, recovery.tuning.baseRegenPerSec) *
			activeAbilityMultiplier;
		const float rawRecovered =
			regenPerSec * dtSec + recovery.regenRemainder;
		const int32_t recovered =
			static_cast<int32_t>(std::floor(rawRecovered));
		recovery.regenRemainder =
			rawRecovered - static_cast<float>(recovered);

		if (recovered <= 0)
		{
			continue;
		}

		const int32_t previousStamina = stats.currentStamina;
		stats.currentStamina =
			std::clamp(stats.currentStamina + recovered, 0, stats.maxStamina);
		if (stats.currentStamina >= stats.maxStamina)
		{
			recovery.regenRemainder = 0.0f;
		}

		if (stats.currentStamina != previousStamina)
		{
			if (DirtyFlagsComp* dirty =
				ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity))
			{
				dirty->MarkDirty(WorldDirtyType::Stat);
			}
		}
	}
}
