#pragma once

#include "GameplayComponentPrerequisites.h"
#include "../../AbilityDef.h"
#include "../../GameplayContentCatalog.h"
#include "../../GameplayContentIds.h"

struct AbilityStateComp : Component
{
	AbilityId abilityId{ InvalidAbilityId };
	uint32_t abilityInstanceId{ 0 };
	float elapsedSec{ 0.0f };
	float directionX{ 0.0f };
	float directionZ{ 0.0f };
	Entity target{ Entity::Null() };

	bool CanIssueAbility() const noexcept
	{
		if (abilityId == InvalidAbilityId)
		{
			return true;
		}

		const GameplayContentCatalogSnapshot* catalog =
			GameplayContentCatalogSnapshot::TryCurrent();
		if (catalog == nullptr)
		{
			return true;
		}

		const AbilityDef* def = catalog->Abilities().Find(abilityId);
		if (def == nullptr)
		{
			return true;
		}

		const float duration = def->timeline.durationSec;
		const float progress =
			(duration > 0.0f) ?
			std::clamp(elapsedSec / duration, 0.0f, 1.0f) :
			1.0f;

		if (progress >= 1.0f)
		{
			return true;
		}

		for (const AbilityTransitionRuleDef& cancel :
			def->transition.cancelRules)
		{
			if (!cancel.aiInterruptible)
			{
				continue;
			}

			if (cancel.windowPolicy == AbilityTransitionWindowPolicy::Always)
			{
				return true;
			}

			const float windowStart =
				cancel.windowStartNormalized.value_or(0.0f);
			const float windowEnd =
				cancel.windowEndNormalized.value_or(1.0f);
			if (progress >= windowStart && progress <= windowEnd)
			{
				return true;
			}
		}

		return false;
	}
};

struct AbilityTimelineAdvanceComp : Component
{
	AbilityId abilityId{ InvalidAbilityId };
	uint32_t abilityInstanceId{ 0 };
	float prevElapsedSec{ 0.0f };
	float currElapsedSec{ 0.0f };
	bool startedThisFrame{ false };
	std::vector<AbilityEventDef> events;
};

struct PendingGameplayEffectApplyComp : Component
{
	GameplayEffectId effectId{ InvalidGameplayEffectId };
};

struct PendingGameplayEffectRemoveComp : Component
{
	GameplayEffectId effectId{ InvalidGameplayEffectId };
};

struct AbilityCooldownEntry
{
	AbilityId abilityId{ InvalidAbilityId };
	float remainingSec{ 0.0f };
};

struct AbilityCooldownStateComp : Component
{
	std::vector<AbilityCooldownEntry> cooldowns;
};

struct AttributeValueEntry
{
	AttributeId attributeId{ InvalidAttributeId };
	float baseValue{ 0.0f };
	float currentValue{ 0.0f };
};

struct AttributeStateComp : Component
{
	std::vector<AttributeValueEntry> values;
};

struct GameplayTagCountEntry
{
	GameplayTagId tagId{ InvalidGameplayTagId };
	uint16_t count{ 0 };
};

struct GameplayTagStateComp : Component
{
	GameplayTagMask dynamicTags{ 0 };
	std::vector<GameplayTagCountEntry> countedTags;
};

struct ActiveGameplayEffectEntry
{
	GameplayEffectId effectId{ InvalidGameplayEffectId };
	uint32_t instanceId{ 0 };
	Entity source{ Entity::Null() };
	float remainingDurationSec{ 0.0f };
	float counterValue{ 0.0f };
	// PeriodicEffect 틱 누산기. tickIntervalSec 을 초과할 때마다 틱 발동 후 감산된다.
	float tickAccumulatorSec{ 0.0f };
	uint16_t stackCount{ 0 };
	uint64_t appliedOrder{ 0 };
};

struct GameplayEffectStateComp : Component
{
	std::vector<ActiveGameplayEffectEntry> activeEffects;
};

// CommitCombatResultSystem 이 킬 판정 시 킬러 엔티티에 적재한다.
// ResolveGameplayEffectStateSystem 이 동일 프레임 후반에 소비하여 버프를 실제 적용한다.
// EffectUser 피처를 가진 캐릭터에만 부착된다.
struct PendingKillBuffGrantComp : Component
{
	std::vector<GameplayEffectId> pendingBuffEffectIds;
};
