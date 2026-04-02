#include "pch.h"
#include "BuffDef.h"
#include "ActionDef.h"

#include <array>
#include <stdexcept>

namespace
{
	const std::array<BuffDef, 4> kBuffDefs =
	{
		BuffDef
		{
			.profile = BuffProfileDef
			{
				.id = BuffId::HpBoost_Low,
				.name = "HpBoost.Low",
				.kind = BuffKind::Positive,
				.familyId = BuffFamilyId::HpBoost,
				.tier = BuffTier::Low,
				.groupId = BuffFamilyId::HpBoost,
				.familyStackPolicy = BuffFamilyStackPolicy::ReplaceWithHigherTier
			},
			.lifetime = BuffLifetimeDef
			{
				.durationPolicy = DurationPolicy::Infinite,
				.defaultDurationSec = 0.0f,
				.tickIntervalSec = std::nullopt,
				.maxStackCount = 1,
				.overlappingPolicy = OverlappingPolicy::Reject,
				.reapplyPolicy = ReapplyPolicy::None
			},
			.effects =
			{
				BuffEffectDef
				{
					.type = BuffEffectType::StatAdd,
					.timing = BuffModifierTiming::Always,
					.stat = BuffStatEffectDef
					{
						.statType = StatType::MaxHp,
						.value = 10.0f
					},
					.stateFlag = std::nullopt,
					.periodic = std::nullopt,
					.condition = std::nullopt
				}
			},
			.applyRequirements = {},
			.removeRules = {}
		},

		BuffDef
		{
			.profile = BuffProfileDef
			{
				.id = BuffId::HpBoost_Mid,
				.name = "HpBoost.Mid",
				.kind = BuffKind::Positive,
				.familyId = BuffFamilyId::HpBoost,
				.tier = BuffTier::Mid,
				.groupId = BuffFamilyId::HpBoost,
				.familyStackPolicy = BuffFamilyStackPolicy::ReplaceWithHigherTier
			},
			.lifetime = BuffLifetimeDef
			{
				.durationPolicy = DurationPolicy::Infinite,
				.defaultDurationSec = 0.0f,
				.tickIntervalSec = std::nullopt,
				.maxStackCount = 1,
				.overlappingPolicy = OverlappingPolicy::Reject,
				.reapplyPolicy = ReapplyPolicy::None
			},
			.effects =
			{
				BuffEffectDef
				{
					.type = BuffEffectType::StatAdd,
					.timing = BuffModifierTiming::Always,
					.stat = BuffStatEffectDef
					{
						.statType = StatType::MaxHp,
						.value = 20.0f
					},
					.stateFlag = std::nullopt,
					.periodic = std::nullopt,
					.condition = std::nullopt
				}
			},
			.applyRequirements = {},
			.removeRules = {}
		},

		BuffDef
		{
			.profile = BuffProfileDef
			{
				.id = BuffId::HpBoost_High,
				.name = "HpBoost.High",
				.kind = BuffKind::Positive,
				.familyId = BuffFamilyId::HpBoost,
				.tier = BuffTier::High,
				.groupId = BuffFamilyId::HpBoost,
				.familyStackPolicy = BuffFamilyStackPolicy::ReplaceWithHigherTier
			},
			.lifetime = BuffLifetimeDef
			{
				.durationPolicy = DurationPolicy::Infinite,
				.defaultDurationSec = 0.0f,
				.tickIntervalSec = std::nullopt,
				.maxStackCount = 1,
				.overlappingPolicy = OverlappingPolicy::Reject,
				.reapplyPolicy = ReapplyPolicy::None
			},
			.effects =
			{
				BuffEffectDef
				{
					.type = BuffEffectType::StatAdd,
					.timing = BuffModifierTiming::Always,
					.stat = BuffStatEffectDef
					{
						.statType = StatType::MaxHp,
						.value = 30.0f
					},
					.stateFlag = std::nullopt,
					.periodic = std::nullopt,
					.condition = std::nullopt
				}
			},
			.applyRequirements = {},
			.removeRules = {}
		},

		BuffDef
		{
			.profile = BuffProfileDef
			{
				.id = BuffId::ParrySuccess,
				.name = "ParrySuccess",
				.kind = BuffKind::Positive,
				.familyId = BuffFamilyId::ParrySuccess,
				.tier = BuffTier::None,
				.groupId = BuffFamilyId::ParrySuccess,
				.familyStackPolicy = BuffFamilyStackPolicy::Independent
			},
			.lifetime = BuffLifetimeDef
			{
				.durationPolicy = DurationPolicy::Timed,
				.defaultDurationSec = 3.0f,
				.tickIntervalSec = std::nullopt,
				.maxStackCount = 1,
				.overlappingPolicy = OverlappingPolicy::Refresh,
				.reapplyPolicy = ReapplyPolicy::RefreshDuration
			},
			.effects =
			{
				BuffEffectDef
				{
					.type = BuffEffectType::StateFlag,
					.timing = BuffModifierTiming::Always,
					.stat = std::nullopt,
					.stateFlag = BuffStateFlagEffectDef
					{
						.flag = GameplayStateFlag::ParrySuccessReady
					},
					.periodic = std::nullopt,
					.condition = std::nullopt
				}
			},
			.applyRequirements =
			{
				BuffApplyRequirementDef
				{
					.type = BuffApplyRequirementType::MissingStateFlag,
					.operand = BuffApplyRequirementOperand
					{
						.scalarCondition = std::nullopt,
						.stateFlagCondition = GameplayStateFlag::ParrySuccessReady,
						.buffFamilyIdCondition = std::nullopt,
						.factionCondition = std::nullopt
					}
				}
			},
			.removeRules =
			{
				BuffRemoveRuleDef
				{
					.type = BuffRemoveRuleType::OnCounterReached,
					.operand = BuffRemoveRuleOperand
					{
						.scalarCondition = 1.0f,
						.stateFlagCondition = std::nullopt,
						.actionIdCondition = std::nullopt,
						.actionKindCondition = ActionKind::Attack,
						.buffFamilyIdCondition = std::nullopt,
						.counterEventCondition = BuffCounterEventType::ActionCommitted
					}
				},
				BuffRemoveRuleDef
				{
					.type = BuffRemoveRuleType::OnDurationExpired,
					.operand = BuffRemoveRuleOperand
					{
						.scalarCondition = std::nullopt,
						.stateFlagCondition = std::nullopt,
						.actionIdCondition = std::nullopt,
						.actionKindCondition = std::nullopt,
						.buffFamilyIdCondition = std::nullopt,
						.counterEventCondition = std::nullopt
					}
				}
			}
		}
	};
}

const BuffDef* FindBuffDef(BuffId id) noexcept
{
	for (const BuffDef& def : kBuffDefs)
	{
		if (def.profile.id == id)
		{
			return &def;
		}
	}

	return nullptr;
}

const BuffDef& GetBuffDef(BuffId id)
{
	const BuffDef* const def = FindBuffDef(id);
	if (def == nullptr)
	{
		throw std::out_of_range("BuffDef was not found.");
	}

	return *def;
}

std::span<const BuffDef> GetBuffDefs() noexcept
{
	return std::span<const BuffDef>(kBuffDefs);
}
