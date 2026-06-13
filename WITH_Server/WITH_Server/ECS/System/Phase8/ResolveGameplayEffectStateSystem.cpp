#include "pch.h"
#include "ResolveGameplayEffectStateSystem.h"

#include "RepComponent.h"
#include "../../Components/GameplayCombatComponents.h"
#include "../../Components/GameplayReplicationComponents.h"
#include "../GameplaySystemUtil.h"
#include "../Phase2/ResolveAbilityStateSystem.h"
#include "CommitAbilityTimelineEventSystem.h"
#include "CommitCombatResultSystem.h"

using namespace GameplaySystemUtil;

namespace
{
	bool DoesAbilityKindMatch(
		const std::optional<std::string>& expectedKind,
		const AbilityDef* abilityDef)
	{
		if (!expectedKind.has_value())
		{
			return true;
		}
		if (abilityDef == nullptr)
		{
			return false;
		}

		if (*expectedKind == "Attack")
		{
			return abilityDef->kind == AbilityKind::Attack;
		}
		if (*expectedKind == "Dodge")
		{
			return abilityDef->kind == AbilityKind::Dodge;
		}
		if (*expectedKind == "Parry")
		{
			return abilityDef->kind == AbilityKind::Parry;
		}
		if (*expectedKind == "Guard")
		{
			return abilityDef->kind == AbilityKind::Guard;
		}
		if (*expectedKind == "UseItem")
		{
			return abilityDef->kind == AbilityKind::UseItem;
		}
		if (*expectedKind == "Reaction")
		{
			return abilityDef->kind == AbilityKind::Reaction;
		}
		if (*expectedKind == "Dead")
		{
			return abilityDef->kind == AbilityKind::Dead;
		}
		if (*expectedKind == "NonCombat")
		{
			return abilityDef->kind == AbilityKind::NonCombat;
		}
		if (*expectedKind == "None")
		{
			return abilityDef->kind == AbilityKind::None;
		}

		return false;
	}

	bool CanApplyEffect(
		const GameplayEffectDef& effectDef,
		const GameplayTagStateComp& tagState)
	{
		const GameplayTagMask ownerTags = BuildGameplayTagMask(&tagState);
		for (const GameplayEffectRequirementDef& requirement :
			effectDef.applyRequirements)
		{
			if (!EvaluateGameplayTagQuery(
					requirement.requiredTags,
					ownerTags,
					true))
			{
				return false;
			}
			if (EvaluateGameplayTagQuery(
					requirement.blockedTags,
					ownerTags,
					false))
			{
				return false;
			}
		}

		return true;
	}

	float ResolveInitialDuration(const GameplayEffectDef& effectDef)
	{
		return effectDef.lifetime.durationPolicy ==
			GameplayEffectDurationPolicy::Timed
			? std::max(0.0f, effectDef.lifetime.defaultDurationSec)
			: 0.0f;
	}

	bool IsSameStackGroup(
		const GameplayEffectDef& lhs,
		const GameplayEffectDef& rhs)
	{
		if (!lhs.stacking.groupKey.empty() || !rhs.stacking.groupKey.empty())
		{
			return lhs.stacking.groupKey == rhs.stacking.groupKey;
		}

		return lhs.id == rhs.id;
	}

	const ActiveGameplayEffectEntry* FindActiveEffectEntry(
		const GameplayEffectStateComp& effectState,
		const GameplayEffectDef& effectDef)
	{
		for (auto it = effectState.activeEffects.rbegin();
			it != effectState.activeEffects.rend();
			++it)
		{
			const GameplayEffectDef* activeDef =
				GameplayContentCatalogSnapshot::Current()
					.GameplayEffects()
					.Find(it->effectId);
			if (activeDef != nullptr && IsSameStackGroup(*activeDef, effectDef))
			{
				return &*it;
			}
		}

		return nullptr;
	}

	void QueueAppliedEffectReplication(
		GameplayEffectReplicationComp* replication,
		const GameplayEffectStateComp& effectState,
		const GameplayEffectDef& effectDef)
	{
		if (replication == nullptr)
		{
			return;
		}

		GameplayEffectAppliedReplicationEvent event{};
		event.effectId = effectDef.id;

		if (const ActiveGameplayEffectEntry* active =
			FindActiveEffectEntry(effectState, effectDef))
		{
			event.instanceId = active->instanceId;
			event.stackCount = std::max<uint16_t>(1, active->stackCount);
			event.remainingDurationSec =
				std::max(0.0f, active->remainingDurationSec);
		}

		replication->pendingAppliedEffects.push_back(event);
	}

	bool HasReplicatedEffectStateChanged(
		const std::vector<ActiveGameplayEffectEntry>& before,
		const std::vector<ActiveGameplayEffectEntry>& after) noexcept
	{
		if (before.size() != after.size())
		{
			return true;
		}

		for (size_t index = 0; index < before.size(); ++index)
		{
			const ActiveGameplayEffectEntry& lhs = before[index];
			const ActiveGameplayEffectEntry& rhs = after[index];
			if (lhs.effectId != rhs.effectId ||
				lhs.instanceId != rhs.instanceId ||
				lhs.source != rhs.source ||
				lhs.stackCount != rhs.stackCount)
			{
				return true;
			}
		}

		return false;
	}

	void AddEffectInstance(
		GameplayEffectStateComp& effectState,
		const GameplayEffectDef& effectDef,
		Entity source,
		uint64_t frameIndex)
	{
		effectState.activeEffects.push_back(ActiveGameplayEffectEntry{
			.effectId = effectDef.id,
			.instanceId = static_cast<uint32_t>(frameIndex & 0xffffffffu),
			.source = source,
			.remainingDurationSec = ResolveInitialDuration(effectDef),
			.counterValue = 0.0f,
			.tickAccumulatorSec = 0.0f,
			.stackCount = 1,
			.appliedOrder = frameIndex
		});
	}

	// attributeModifiers 에 정의된 단일 수정자를 CombatStatStateComp 에 즉시 반영한다.
	// Add: 정수 필드는 lround, 부동소수 필드는 직접 가산.
	// Multiply: 각 필드에 계수를 곱한다.
	void ApplyAttributeModifierToStats(
		const AttributeModifierDef& modifier,
		CombatStatStateComp& stats) noexcept
	{
		const AttributeDef* attrDef =
			GameplayContentCatalogSnapshot::Current()
				.Attributes()
				.Find(modifier.attributeId);
		if (attrDef == nullptr)
		{
			return;
		}

		const std::string_view key = attrDef->key;

		auto applyInt = [&modifier](int32_t& field)
		{
			if (modifier.op == AttributeModifierOp::Add)
			{
				field += static_cast<int32_t>(std::lround(modifier.value));
			}
			else if (modifier.op == AttributeModifierOp::Multiply)
			{
				field = static_cast<int32_t>(
					std::lround(static_cast<float>(field) * modifier.value));
			}
		};

		auto applyFloat = [&modifier](float& field)
		{
			if (modifier.op == AttributeModifierOp::Add)
			{
				field += modifier.value;
			}
			else if (modifier.op == AttributeModifierOp::Multiply)
			{
				field *= modifier.value;
			}
		};

		if (key == "Attribute.Hp")
		{
			applyInt(stats.currentHp);
			stats.currentHp = std::clamp(stats.currentHp, 0, stats.maxHp);
		}
		else if (key == "Attribute.MaxHp")
		{
			const int32_t previousMaxHp = stats.maxHp;
			applyInt(stats.maxHp);
			stats.maxHp = std::max(0, stats.maxHp);

			const int32_t maxHpDelta = stats.maxHp - previousMaxHp;
			if (maxHpDelta > 0)
			{
				stats.currentHp += maxHpDelta;
			}
			stats.currentHp = std::clamp(stats.currentHp, 0, stats.maxHp);
		}
		else if (key == "Attribute.Stamina")
		{
			applyInt(stats.currentStamina);
			stats.currentStamina =
				std::clamp(stats.currentStamina, 0, stats.maxStamina);
		}
		else if (key == "Attribute.MaxStamina")
		{
			applyInt(stats.maxStamina);
		}
		else if (key == "Attribute.Poise")
		{
			applyInt(stats.currentPoise);
			stats.currentPoise =
				std::clamp(stats.currentPoise, 0, stats.maxPoise);
		}
		else if (key == "Attribute.MaxPoise")
		{
			applyInt(stats.maxPoise);
		}
		else if (key == "Attribute.AttackPower")
		{
			applyInt(stats.attackPower);
		}
		else if (key == "Attribute.Defense")
		{
			applyInt(stats.defense);
		}
		else if (key == "Attribute.MoveSpeed")
		{
			applyFloat(stats.moveSpeed);
		}
		else if (key == "Attribute.AttackSpeed")
		{
			applyFloat(stats.attackSpeed);
		}
	}

	// effectDef 의 attributeModifiers 를 stats 에 한 스택분 반영한다.
	void ApplyModifiersForOneStack(
		const GameplayEffectDef& effectDef,
		CombatStatStateComp& stats) noexcept
	{
		for (const AttributeModifierDef& modifier : effectDef.attributeModifiers)
		{
			ApplyAttributeModifierToStats(modifier, stats);
		}
	}

	// effectState 에 effectDef 를 적용한다.
	// stats 가 nullptr 이 아니면 Instant 즉시 적용 및 스택 증가 시 스탯 반영을 수행한다.
	// 반환값: 스탯 변경이 일어났으면 true.
	bool ApplyEffect(
		GameplayEffectStateComp& effectState,
		const GameplayEffectDef& effectDef,
		Entity source,
		uint64_t frameIndex,
		CombatStatStateComp* stats)
	{
		// Instant: activeEffects 에 올리지 않고 수정자만 즉시 반영한다.
		if (effectDef.lifetime.durationPolicy ==
			GameplayEffectDurationPolicy::Instant)
		{
			if (stats != nullptr && !effectDef.attributeModifiers.empty())
			{
				ApplyModifiersForOneStack(effectDef, *stats);
				return true;
			}
			return false;
		}

		auto matchingEntry = effectState.activeEffects.end();
		for (auto it = effectState.activeEffects.begin();
			it != effectState.activeEffects.end();
			++it)
		{
			const GameplayEffectDef* activeDef =
				GameplayContentCatalogSnapshot::Current()
					.GameplayEffects()
					.Find(it->effectId);
			if (activeDef != nullptr && IsSameStackGroup(*activeDef, effectDef))
			{
				matchingEntry = it;
				break;
			}
		}

		bool statsChanged = false;

		switch (effectDef.stacking.stackPolicy)
		{
		case GameplayEffectStackPolicy::Refresh:
			if (matchingEntry != effectState.activeEffects.end())
			{
				matchingEntry->effectId = effectDef.id;
				matchingEntry->source = source;
				matchingEntry->remainingDurationSec =
					ResolveInitialDuration(effectDef);
				matchingEntry->counterValue = 0.0f;
				matchingEntry->stackCount = std::max<uint16_t>(
					1,
					std::min<uint16_t>(
						matchingEntry->stackCount,
						effectDef.stacking.maxStackCount));
				return false;
			}
			break;
		case GameplayEffectStackPolicy::Stack:
			if (matchingEntry != effectState.activeEffects.end())
			{
				matchingEntry->source = source;
				matchingEntry->remainingDurationSec =
					ResolveInitialDuration(effectDef);
				const uint16_t previousStackCount = matchingEntry->stackCount;
				matchingEntry->stackCount = std::min<uint16_t>(
					static_cast<uint16_t>(matchingEntry->stackCount + 1),
					std::max<uint16_t>(1, effectDef.stacking.maxStackCount));
				// 실제로 스택이 증가한 경우에만 수정자를 한 스택분 추가 반영한다.
				if (stats != nullptr &&
					matchingEntry->stackCount > previousStackCount &&
					!effectDef.attributeModifiers.empty())
				{
					ApplyModifiersForOneStack(effectDef, *stats);
					statsChanged = true;
				}
				return statsChanged;
			}
			break;
		case GameplayEffectStackPolicy::Replace:
			effectState.activeEffects.erase(
				std::remove_if(
					effectState.activeEffects.begin(),
					effectState.activeEffects.end(),
					[&effectDef](const ActiveGameplayEffectEntry& entry)
					{
						const GameplayEffectDef* activeDef =
							GameplayContentCatalogSnapshot::Current()
								.GameplayEffects()
								.Find(entry.effectId);
						return activeDef != nullptr &&
							IsSameStackGroup(*activeDef, effectDef);
					}),
				effectState.activeEffects.end());
			break;
		case GameplayEffectStackPolicy::Independent:
		default:
			break;
		}

		AddEffectInstance(effectState, effectDef, source, frameIndex);

		// 신규 인스턴스(첫 스택)에 대한 수정자 반영.
		if (stats != nullptr && !effectDef.attributeModifiers.empty())
		{
			ApplyModifiersForOneStack(effectDef, *stats);
			statsChanged = true;
		}

		return statsChanged;
	}

	void RebuildGrantedTags(
		const GameplayEffectStateComp& effectState,
		GameplayTagStateComp& tagState)
	{
		tagState.countedTags.clear();

		for (const ActiveGameplayEffectEntry& entry : effectState.activeEffects)
		{
			const GameplayEffectDef* effectDef =
				GameplayContentCatalogSnapshot::Current()
					.GameplayEffects()
					.Find(entry.effectId);
			if (effectDef == nullptr)
			{
				continue;
			}

			for (const GameplayTagModifierDef& modifier :
				effectDef->tagModifiers)
			{
				if (modifier.tagId == InvalidGameplayTagId ||
					modifier.op != GameplayTagModifierOp::Add)
				{
					continue;
				}

				auto existing = std::find_if(
					tagState.countedTags.begin(),
					tagState.countedTags.end(),
					[&modifier](const GameplayTagCountEntry& count)
					{
						return count.tagId == modifier.tagId;
					});
				if (existing != tagState.countedTags.end())
				{
					++existing->count;
				}
				else
				{
					tagState.countedTags.push_back(GameplayTagCountEntry{
						.tagId = modifier.tagId,
						.count = 1
					});
				}
			}
		}

		GameplayTagMask removedMask = 0;
		for (const ActiveGameplayEffectEntry& entry : effectState.activeEffects)
		{
			const GameplayEffectDef* effectDef =
				GameplayContentCatalogSnapshot::Current()
					.GameplayEffects()
					.Find(entry.effectId);
			if (effectDef == nullptr)
			{
				continue;
			}

			for (const GameplayTagModifierDef& modifier :
				effectDef->tagModifiers)
			{
				if (modifier.op == GameplayTagModifierOp::Remove)
				{
					removedMask |= GameplayTagBit(modifier.tagId);
				}
			}
		}

		if (removedMask == 0)
		{
			return;
		}

		tagState.dynamicTags &= ~removedMask;
		tagState.countedTags.erase(
			std::remove_if(
				tagState.countedTags.begin(),
				tagState.countedTags.end(),
				[removedMask](const GameplayTagCountEntry& count)
				{
					return (GameplayTagBit(count.tagId) & removedMask) != 0;
				}),
			tagState.countedTags.end());
	}

	bool ShouldRemoveForRule(
		const GameplayEffectRemoveRuleDef& rule,
		const GameplayEffectDef& effectDef,
		const ActiveGameplayEffectEntry& entry,
		const GameplayTagStateComp& tagState,
		const AbilityDef* committedAbilityDef,
		bool abilityCommitted)
	{
		const GameplayTagMask ownerTags = BuildGameplayTagMask(&tagState);
		const bool tagConditionMatches = EvaluateGameplayTagQuery(
			rule.removeWhenTagsMatch,
			ownerTags,
			true);
		if (!tagConditionMatches)
		{
			return false;
		}

		switch (rule.trigger)
		{
		case GameplayEffectRemoveRuleTrigger::DurationExpired:
			return effectDef.lifetime.durationPolicy ==
				GameplayEffectDurationPolicy::Timed &&
				entry.remainingDurationSec <= 0.0f;
		case GameplayEffectRemoveRuleTrigger::CounterReached:
			return abilityCommitted &&
				rule.counterEvent ==
					GameplayEffectCounterEvent::AbilityCommitted &&
				DoesAbilityKindMatch(rule.abilityKind, committedAbilityDef) &&
				rule.counterThreshold.has_value() &&
				entry.counterValue >= *rule.counterThreshold;
		case GameplayEffectRemoveRuleTrigger::TagQueryMatched:
			return true;
		default:
			return false;
		}
	}
}

const StaticSystemMetaStorage<13, 0, 3>
ResolveGameplayEffectStateSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<ResolveGameplayEffectStateSystem>(),
		"ResolveGameplayEffectStateSystem",
		std::array<AccessSpec, 13>
	{
		WriteImmediate(ComponentRes<GameplayEffectStateComp>()),
		WriteImmediate(ComponentRes<GameplayTagStateComp>()),
		WriteImmediate(ComponentRes<PendingGameplayEffectApplyComp>()),
		WriteImmediate(ComponentRes<PendingGameplayEffectRemoveComp>()),
		ReadImmediate(ComponentRes<AbilityTimelineAdvanceComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		ReadImmediate(ComponentRes<AbilityStateComp>()),
		WriteImmediate(ComponentRes<PendingKillBuffGrantComp>()),
		WriteImmediate(ComponentRes<CombatStatStateComp>()),
		WriteImmediate(ComponentRes<AbilityInterruptQueueComp>()),
		WriteImmediate(ComponentRes<DirtyFlagsComp>()),
		WriteImmediate(ComponentRes<GameplayEffectReplicationComp>()),
	},
		std::array<SystemTag, 0>{},
		std::array<SystemTag, 3>
		{
			SysTag<ResolveAbilityStateSystem>(),
			SysTag<CommitCombatResultSystem>(),
			SysTag<CommitAbilityTimelineEventSystem>(),
		});

void ResolveGameplayEffectStateSystem::Execute(SystemContext& ctx)
{
	const float dtSec = static_cast<float>(std::max(0.0, ctx.dtSec));

	for (auto [
		entity,
		effectState,
		tagState,
		pendingApply,
		pendingRemove,
		advance] :
		ctx.ecs.View<
			GameplayEffectStateComp,
			GameplayTagStateComp,
			PendingGameplayEffectApplyComp,
			PendingGameplayEffectRemoveComp,
			AbilityTimelineAdvanceComp>())
	{
		const std::vector<ActiveGameplayEffectEntry> activeEffectsBefore =
			effectState.activeEffects;
		GameplayEffectReplicationComp* effectReplication =
			ctx.ecs.GetMutableComponent<GameplayEffectReplicationComp>(entity);
		const size_t pendingAppliedCountBefore =
			effectReplication != nullptr
			? effectReplication->pendingAppliedEffects.size()
			: 0;

		if (HasBlockingPendingState(ctx.ecs, entity))
		{
			pendingApply = {};
			pendingRemove = {};
			effectState.activeEffects.clear();
			RebuildGrantedTags(effectState, tagState);

			if (PendingKillBuffGrantComp* killBuffGrant =
				ctx.ecs.GetMutableComponent<PendingKillBuffGrantComp>(entity))
			{
				killBuffGrant->pendingBuffEffectIds.clear();
			}
			if (effectReplication != nullptr)
			{
				effectReplication->pendingAppliedEffects.clear();
			}

			continue;
		}

		// ── (1) Timed 이펙트 지속 시간 감산 ──────────────────────────────────
		for (ActiveGameplayEffectEntry& entry : effectState.activeEffects)
		{
			const GameplayEffectDef* effectDef =
				GameplayContentCatalogSnapshot::Current()
					.GameplayEffects()
					.Find(entry.effectId);
			if (effectDef != nullptr &&
				effectDef->lifetime.durationPolicy ==
					GameplayEffectDurationPolicy::Timed)
			{
				entry.remainingDurationSec =
					std::max(0.0f, entry.remainingDurationSec - dtSec);
			}
		}

		// ── (2) PeriodicEffect 틱 누산 및 발동 ───────────────────────────────
		CombatStatStateComp* stats =
			ctx.ecs.GetMutableComponent<CombatStatStateComp>(entity);
		bool statsModifiedByTick = false;

		for (ActiveGameplayEffectEntry& entry : effectState.activeEffects)
		{
			const GameplayEffectDef* effectDef =
				GameplayContentCatalogSnapshot::Current()
					.GameplayEffects()
					.Find(entry.effectId);
			if (effectDef == nullptr ||
				!effectDef->lifetime.tickIntervalSec.has_value() ||
				effectDef->periodicEffects.empty())
			{
				continue;
			}

			const float tickInterval = *effectDef->lifetime.tickIntervalSec;
			if (tickInterval <= 0.0f)
			{
				continue;
			}

			entry.tickAccumulatorSec += dtSec;

			while (entry.tickAccumulatorSec >= tickInterval)
			{
				entry.tickAccumulatorSec -= tickInterval;

				// tickDurationSec 체크를 위한 경과 시간:
				// Refresh 재적용 시에도 remainingDurationSec 이 리셋되므로
				// 이펙트가 처음 적용된 시점부터의 경과 시간으로 동작한다.
				const float elapsedSec =
					effectDef->lifetime.defaultDurationSec -
					entry.remainingDurationSec;

				for (const PeriodicEffectDef& periodicEffect :
					effectDef->periodicEffects)
				{
					// 틱 윈도우 초과 여부 확인.
					if (periodicEffect.tickDurationSec.has_value() &&
						elapsedSec > *periodicEffect.tickDurationSec)
					{
						continue;
					}

					if (periodicEffect.kind != PeriodicEffectKind::AttributeDelta ||
						stats == nullptr)
					{
						continue;
					}

					// 스택 수를 반영한 델타를 한 번에 적용한다.
					AttributeModifierDef scaledModifier{};
					scaledModifier.attributeId = periodicEffect.attributeId;
					scaledModifier.op = AttributeModifierOp::Add;
					scaledModifier.value =
						periodicEffect.value *
						static_cast<float>(entry.stackCount);

					ApplyAttributeModifierToStats(scaledModifier, *stats);
					statsModifiedByTick = true;
				}
			}
		}

		// 틱으로 HP 가 0 이하가 된 경우 사망 인터럽트를 발행한다.
		if (statsModifiedByTick && stats != nullptr && stats->currentHp <= 0)
		{
			if (AbilityInterruptQueueComp* interruptQueue =
				ctx.ecs.GetMutableComponent<AbilityInterruptQueueComp>(entity))
			{
				interruptQueue->events.push_back(AbilityInterruptEvent{
					.cause = AbilityTransitionCause::OnAttributeZero,
					.instigator = Entity::Null(),
					.frameIndex = ctx.runtime.FrameIndex(),
					.priority = 1000
				});
			}
		}

		// ── (3) 명시적 Remove 처리 ───────────────────────────────────────────
		if (pendingRemove.effectId != InvalidGameplayEffectId)
		{
			effectState.activeEffects.erase(
				std::remove_if(
					effectState.activeEffects.begin(),
					effectState.activeEffects.end(),
					[&pendingRemove](const ActiveGameplayEffectEntry& entry)
					{
						return entry.effectId == pendingRemove.effectId;
					}),
				effectState.activeEffects.end());
			pendingRemove = {};
		}

		// ── (4) DurationExpired 제거 (1차: 어빌리티 이벤트 이전) ─────────────
		effectState.activeEffects.erase(
			std::remove_if(
				effectState.activeEffects.begin(),
				effectState.activeEffects.end(),
				[&tagState](const ActiveGameplayEffectEntry& entry)
				{
					const GameplayEffectDef* effectDef =
						GameplayContentCatalogSnapshot::Current()
							.GameplayEffects()
							.Find(entry.effectId);
					if (effectDef == nullptr)
					{
						return true;
					}

					for (const GameplayEffectRemoveRuleDef& rule :
						effectDef->removeRules)
					{
						if (rule.trigger ==
								GameplayEffectRemoveRuleTrigger::DurationExpired &&
							ShouldRemoveForRule(
								rule,
								*effectDef,
								entry,
								tagState,
								nullptr,
								false))
						{
							return true;
						}
					}

					return false;
				}),
			effectState.activeEffects.end());
		RebuildGrantedTags(effectState, tagState);

		// ── (5) PendingApply 처리 ─────────────────────────────────────────────
		if (pendingApply.effectId != InvalidGameplayEffectId)
		{
			const GameplayEffectDef* effectDef =
				GameplayContentCatalogSnapshot::Current()
					.GameplayEffects()
					.Find(pendingApply.effectId);
			if (effectDef != nullptr && CanApplyEffect(*effectDef, tagState))
			{
				const bool changed = ApplyEffect(
					effectState,
					*effectDef,
					entity,
					ctx.runtime.FrameIndex(),
					stats);
				if (changed)
				{
					statsModifiedByTick = true; // dirty 마킹을 위해 재활용
				}
				QueueAppliedEffectReplication(
					effectReplication,
					effectState,
					*effectDef);
			}
			pendingApply = {};
		}

		// ── (6) 킬 버프 소비 ──────────────────────────────────────────────────
		if (PendingKillBuffGrantComp* killBuffGrant =
			ctx.ecs.GetMutableComponent<PendingKillBuffGrantComp>(entity))
		{
			for (const GameplayEffectId buffEffectId :
				killBuffGrant->pendingBuffEffectIds)
			{
				const GameplayEffectDef* effectDef =
					GameplayContentCatalogSnapshot::Current()
						.GameplayEffects()
						.Find(buffEffectId);
				if (effectDef != nullptr && CanApplyEffect(*effectDef, tagState))
				{
					const bool changed = ApplyEffect(
						effectState,
						*effectDef,
						entity,
						ctx.runtime.FrameIndex(),
						stats);
					if (changed)
					{
						statsModifiedByTick = true;
					}
					QueueAppliedEffectReplication(
						effectReplication,
						effectState,
						*effectDef);
				}
			}
			killBuffGrant->pendingBuffEffectIds.clear();
		}

		// ── (7) 스탯 변경 시 Dirty 마킹 ──────────────────────────────────────
		if (statsModifiedByTick)
		{
			if (DirtyFlagsComp* dirty =
				ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity))
			{
				dirty->MarkDirty(WorldDirtyType::Stat);
			}
		}

		// ── (8) AbilityCommitted 카운터 갱신 ─────────────────────────────────
		const bool abilityCommitted =
			advance.startedThisFrame &&
			advance.abilityId != InvalidAbilityId;
		const AbilityDef* committedAbilityDef =
			abilityCommitted
			? GameplayContentCatalogSnapshot::Current()
				.Abilities()
				.Find(advance.abilityId)
			: nullptr;

		if (abilityCommitted)
		{
			for (ActiveGameplayEffectEntry& entry : effectState.activeEffects)
			{
				const GameplayEffectDef* effectDef =
					GameplayContentCatalogSnapshot::Current()
						.GameplayEffects()
						.Find(entry.effectId);
				if (effectDef == nullptr)
				{
					continue;
				}

				for (const GameplayEffectRemoveRuleDef& rule :
					effectDef->removeRules)
				{
					if (rule.trigger ==
							GameplayEffectRemoveRuleTrigger::CounterReached &&
						rule.counterEvent ==
							GameplayEffectCounterEvent::AbilityCommitted &&
						DoesAbilityKindMatch(
							rule.abilityKind,
							committedAbilityDef))
					{
						entry.counterValue += 1.0f;
						break;
					}
				}
			}
		}

		// ── (9) 최종 제거 (CounterReached 포함) ──────────────────────────────
		effectState.activeEffects.erase(
			std::remove_if(
				effectState.activeEffects.begin(),
				effectState.activeEffects.end(),
				[&tagState, committedAbilityDef, abilityCommitted](
					const ActiveGameplayEffectEntry& entry)
				{
					const GameplayEffectDef* effectDef =
						GameplayContentCatalogSnapshot::Current()
							.GameplayEffects()
							.Find(entry.effectId);
					if (effectDef == nullptr)
					{
						return true;
					}

					for (const GameplayEffectRemoveRuleDef& rule :
						effectDef->removeRules)
					{
						if (ShouldRemoveForRule(
								rule,
								*effectDef,
								entry,
								tagState,
								committedAbilityDef,
								abilityCommitted))
						{
							return true;
						}
					}

					return false;
				}),
			effectState.activeEffects.end());

		RebuildGrantedTags(effectState, tagState);

		const bool activeEffectStateChanged =
			HasReplicatedEffectStateChanged(
				activeEffectsBefore,
				effectState.activeEffects);
		const bool appliedEffectQueued =
			effectReplication != nullptr &&
			effectReplication->pendingAppliedEffects.size() >
				pendingAppliedCountBefore;
		if (effectReplication != nullptr &&
			(activeEffectStateChanged || appliedEffectQueued))
		{
			++effectReplication->revision;
			if (DirtyFlagsComp* dirty =
				ctx.ecs.GetMutableComponent<DirtyFlagsComp>(entity))
			{
				dirty->MarkDirty(WorldDirtyType::GameplayEffect);
			}
		}
	}
}
