#include "pch.h"
#include "ResolveGameplayEffectStateSystem.h"

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
			.stackCount = 1,
			.appliedOrder = frameIndex
		});
	}

	void ApplyEffect(
		GameplayEffectStateComp& effectState,
		const GameplayEffectDef& effectDef,
		Entity source,
		uint64_t frameIndex)
	{
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
				return;
			}
			break;
		case GameplayEffectStackPolicy::Stack:
			if (matchingEntry != effectState.activeEffects.end())
			{
				matchingEntry->source = source;
				matchingEntry->remainingDurationSec =
					ResolveInitialDuration(effectDef);
				matchingEntry->stackCount = std::min<uint16_t>(
					static_cast<uint16_t>(matchingEntry->stackCount + 1),
					std::max<uint16_t>(1, effectDef.stacking.maxStackCount));
				return;
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

const StaticSystemMetaStorage<8, 0, 3>
ResolveGameplayEffectStateSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<ResolveGameplayEffectStateSystem>(),
		"ResolveGameplayEffectStateSystem",
		std::array<AccessSpec, 8>
	{
		WriteImmediate(ComponentRes<GameplayEffectStateComp>()),
		WriteImmediate(ComponentRes<GameplayTagStateComp>()),
		WriteImmediate(ComponentRes<PendingGameplayEffectApplyComp>()),
		WriteImmediate(ComponentRes<PendingGameplayEffectRemoveComp>()),
		ReadImmediate(ComponentRes<AbilityTimelineAdvanceComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
		ReadImmediate(ComponentRes<AbilityStateComp>()),
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
		if (HasBlockingPendingState(ctx.ecs, entity))
		{
			pendingApply = {};
			pendingRemove = {};
			effectState.activeEffects.clear();
			RebuildGrantedTags(effectState, tagState);
			continue;
		}

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

		if (pendingApply.effectId != InvalidGameplayEffectId)
		{
			const GameplayEffectDef* effectDef =
				GameplayContentCatalogSnapshot::Current()
					.GameplayEffects()
					.Find(pendingApply.effectId);
			if (effectDef != nullptr && CanApplyEffect(*effectDef, tagState))
			{
				ApplyEffect(
					effectState,
					*effectDef,
					entity,
					ctx.runtime.FrameIndex());
			}
			pendingApply = {};
		}

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
	}
}
