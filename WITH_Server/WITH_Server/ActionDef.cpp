#include "pch.h"
#include "ActionDef.h"

#include <array>
#include <stdexcept>

namespace
{
	constexpr ComboGroupId kKnightLightComboGroupId = 1;

	const std::array<ActionDef, 19> kActionDefs =
	{
		ActionDef
		{
			.id = ActionId::Knight_LightAttack1,
			.name = "Knight.LightAttack1",
			.characterId = CharacterId::Knight,
			.kind = ActionKind::Attack,
			.playerInput = PlayerActionInput::LightAttack,
			.comboGroupId = kKnightLightComboGroupId,
			.comboIndex = 1,
			.duration = 1.16f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements =
			{
				ActionRequestRequirementDef                          
				{
					.type = ActionRequestRequirementType::HasEnoughStamina,
					.scalar = 0,
					.stateFlag = std::nullopt
				},
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::IsGrounded,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				}
			},
			.resourceCosts =
			{
				ActionResourceCostDef
				{
					.type = ActionResourceType::Stamina,
					.consumeTiming = ActionResourceConsumeTiming::OnRequest,
					.amount = 0
				}
			},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHitReceived,
						.toActionId = ActionId::Knight_Hit,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 100
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnParried,
						.toActionId = ActionId::Knight_Stun,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 200
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Knight_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules =
				{
					ActionCancelRule
					{
						.cancelKind = ActionCancelKind::Combo,
						.toActionId = ActionId::Knight_LightAttack2,
						.windowPolicy = ActionWindowPolicy::Range,
						.windowStartNormalized = 0.45f,
						.windowEndNormalized = 0.75f,
						.priority = 100
					},
					ActionCancelRule
					{
						.cancelKind = ActionCancelKind::DodgeCancel,
						.toActionId = ActionId::Knight_Dodge,
						.windowPolicy = ActionWindowPolicy::Range,
						.windowStartNormalized = 0.50f,
						.windowEndNormalized = 0.80f,
						.priority = 90
					}
				}
			},
			.combatWindows =
			{
				ActionCombatWindowDef
				{
					.windowType = CombatWindowType::Attack,
					.startNormalized = 0.3f,
					.endNormalized = 1.0f,
					.appliesTo = ActionCombatApplyTo::FrontPhysical,
					.spatialFilter = std::nullopt,
					.effect = CombatEffectDef
					{
						.type = CombatEffectType::AttackHit,
						.attackHit = AttackCombatEffectDef
						{
							.damageScale = 1.0f,
							.bonusDamage = 2.0f,
							.staminaDamageScale = 0.5f,
							.bonusStaminaDamage = 3.0f,
							.poiseDamageScale = 0.0f,
							.bonusPoiseDamage = 0.0f,
							.knockbackDistance = 0.35f,
							.hitStopSec = 0.05f,
							.parryable = true,
							.guardable = true
						},
						.parryResponse = std::nullopt,
						.guardResponse = std::nullopt
					}
				}
			},
			.events = {},
			.moveSegments = {}
		},

		ActionDef
		{
			.id = ActionId::Knight_LightAttack2,
			.name = "Knight.LightAttack2",
			.characterId = CharacterId::Knight,
			.kind = ActionKind::Attack,
			.playerInput = PlayerActionInput::LightAttack,
			.comboGroupId = kKnightLightComboGroupId,
			.comboIndex = 2,
			.duration = 1.06f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements =
			{
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::HasEnoughStamina,
					.scalar = 0,
					.stateFlag = std::nullopt
				},
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::IsGrounded,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				}
			},
			.resourceCosts =
			{
				ActionResourceCostDef
				{
					.type = ActionResourceType::Stamina,
					.consumeTiming = ActionResourceConsumeTiming::OnRequest,
					.amount = 0
				}
			},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHitReceived,
						.toActionId = ActionId::Knight_Hit,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 100
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnParried,
						.toActionId = ActionId::Knight_Stun,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 200
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Knight_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules =
				{
					ActionCancelRule
					{
						.cancelKind = ActionCancelKind::Combo,
						.toActionId = ActionId::Knight_LightAttack3,
						.windowPolicy = ActionWindowPolicy::Range,
						.windowStartNormalized = 0.48f,
						.windowEndNormalized = 0.78f,
						.priority = 100
					},
					ActionCancelRule
					{
						.cancelKind = ActionCancelKind::DodgeCancel,
						.toActionId = ActionId::Knight_Dodge,
						.windowPolicy = ActionWindowPolicy::Range,
						.windowStartNormalized = 0.55f,
						.windowEndNormalized = 0.85f,
						.priority = 90
					}
				}
			},
			.combatWindows =
			{
				ActionCombatWindowDef
				{
					.windowType = CombatWindowType::Attack,
					.startNormalized = 0.3f,
					.endNormalized = 1.0f,
					.appliesTo = ActionCombatApplyTo::FrontPhysical,
					.spatialFilter = std::nullopt,
					.effect = CombatEffectDef
					{
						.type = CombatEffectType::AttackHit,
						.attackHit = AttackCombatEffectDef
						{
							.damageScale = 1.2f,
							.bonusDamage = 4.0f,
							.staminaDamageScale = 0.7f,
							.bonusStaminaDamage = 3.0f,
							.poiseDamageScale = 0.0f,
							.bonusPoiseDamage = 0.0f,
							.knockbackDistance = 0.45f,
							.hitStopSec = 0.06f,
							.parryable = true,
							.guardable = true
						},
						.parryResponse = std::nullopt,
						.guardResponse = std::nullopt
					}
				}
			},
			.events = {},
			.moveSegments =
			{
				ActionMovementSegmentDef
				{
					.startNormalized = 0 / 47.0f,
					.endNormalized = 19 / 47.0f,
					.horizontalMoveMode = HorizontalMovementMode::ForwardFixedDistance,
					.moveDistance = 54.0f / 100.0f,
					.rotationMode = RotationMode::None,
					.rotationRate = std::nullopt,
					.verticalMoveMode = VerticalMovementMode::None,
					.verticalAmount = std::nullopt,
					.dirPolicy = DirectionPolicy::ActionStartInput,
					.dirSampleTiming = DirectionSampleTiming::OnSegmentStart
				}
			}
		},

		ActionDef
		{
			.id = ActionId::Knight_LightAttack3,
			.name = "Knight.LightAttack3",
			.characterId = CharacterId::Knight,
			.kind = ActionKind::Attack,
			.playerInput = PlayerActionInput::LightAttack,
			.comboGroupId = kKnightLightComboGroupId,
			.comboIndex = 3,
			.duration = 1.53f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements =
			{
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::HasEnoughStamina,
					.scalar = 15.0f,
					.stateFlag = std::nullopt
				},
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::IsGrounded,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				}
			},
			.resourceCosts =
			{
				ActionResourceCostDef
				{
					.type = ActionResourceType::Stamina,
					.consumeTiming = ActionResourceConsumeTiming::OnRequest,
					.amount = 15.0f
				}
			},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHitReceived,
						.toActionId = ActionId::Knight_Hit,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 100
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnParried,
						.toActionId = ActionId::Knight_Stun,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 200
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Knight_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules =
				{
					ActionCancelRule
					{
						.cancelKind = ActionCancelKind::DodgeCancel,
						.toActionId = ActionId::Knight_Dodge,
						.windowPolicy = ActionWindowPolicy::Range,
						.windowStartNormalized = 0.62f,
						.windowEndNormalized = 0.88f,
						.priority = 90
					}
				}
			},
			.combatWindows =
			{
				ActionCombatWindowDef
				{
					.windowType = CombatWindowType::Attack,
					.startNormalized = 0.3f,
					.endNormalized = 1.0f,
					.appliesTo = ActionCombatApplyTo::FrontPhysical,
					.spatialFilter = std::nullopt,
					.effect = CombatEffectDef
					{
						.type = CombatEffectType::AttackHit,
						.attackHit = AttackCombatEffectDef
						{
							.damageScale = 1.5f,
							.bonusDamage = 7.0f,
							.staminaDamageScale = 1.0f,
							.bonusStaminaDamage = 4.0f,
							.poiseDamageScale = 0.0f,
							.bonusPoiseDamage = 0.0f,
							.knockbackDistance = 0.70f,
							.hitStopSec = 0.07f,
							.parryable = true,
							.guardable = true
						},
						.parryResponse = std::nullopt,
						.guardResponse = std::nullopt
					},
				}
			},
			.events = {},
			.moveSegments =
			{
				ActionMovementSegmentDef
				{
					.startNormalized = 0 / 47.0f,
					.endNormalized = 19 / 47.0f,
					.horizontalMoveMode = HorizontalMovementMode::ForwardFixedDistance,
					.moveDistance = 74 / 100.0f,
					.rotationMode = RotationMode::None,
					.rotationRate = std::nullopt,
					.verticalMoveMode = VerticalMovementMode::None,
					.verticalAmount = std::nullopt,
					.dirPolicy = DirectionPolicy::ActionStartInput,
					.dirSampleTiming = DirectionSampleTiming::OnSegmentStart
				}
			}
		},

		ActionDef
		{
			.id = ActionId::Knight_HeavyAttack,
			.name = "Knight.HeavyAttack",
			.characterId = CharacterId::Knight,
			.kind = ActionKind::Attack,
			.playerInput = PlayerActionInput::HeavAttack,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = 1.30f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements =
			{
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::HasEnoughStamina,
					.scalar = 0,
					.stateFlag = std::nullopt
				},
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::IsGrounded,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				}
			},
			.resourceCosts =
			{
				ActionResourceCostDef
				{
					.type = ActionResourceType::Stamina,
					.consumeTiming = ActionResourceConsumeTiming::OnRequest,
					.amount = 0
				}
			},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHitReceived,
						.toActionId = ActionId::Knight_Hit,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 100
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnParried,
						.toActionId = ActionId::Knight_Stun,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 200
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Knight_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules = {}
			},
			.combatWindows =
			{
				ActionCombatWindowDef
				{
					.windowType = CombatWindowType::Attack,
					.startNormalized = 0.3f,
					.endNormalized = 1.0f,
					.appliesTo = ActionCombatApplyTo::FrontPhysical,
					.spatialFilter = std::nullopt,
					.effect = CombatEffectDef
					{
						.type = CombatEffectType::AttackHit,
						.attackHit = AttackCombatEffectDef
						{
							.damageScale = 2.5f,
							.bonusDamage = 9.0f,
							.staminaDamageScale = 1.5f,
							.bonusStaminaDamage = 5.0f,
							.poiseDamageScale = 0.0f,
							.bonusPoiseDamage = 0.0f,
							.knockbackDistance = 1.10f,
							.hitStopSec = 0.09f,
							.parryable = true,
							.guardable = true
						},
						.parryResponse = std::nullopt,
						.guardResponse = std::nullopt
					}
				}
			},
			.events = {},
			.moveSegments = {}
		},

		ActionDef
		{
			.id = ActionId::Knight_SpecialAttack,
			.name = "Knight.SpecialAttack",
			.characterId = CharacterId::Knight,
			.kind = ActionKind::Attack,
			.playerInput = PlayerActionInput::None,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = 3.13f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements =
			{
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::HasEnoughStamina,
					.scalar = 0,
					.stateFlag = std::nullopt
				},
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::HasStateFlag,
					.scalar = std::nullopt,
					.stateFlag = GameplayStateFlag::ParrySuccessReady
				},
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::IsGrounded,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				}
			},
			.resourceCosts =
			{
				ActionResourceCostDef
				{
					.type = ActionResourceType::Stamina,
					.consumeTiming = ActionResourceConsumeTiming::OnRequest,
					.amount = 0
				}
			},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHitReceived,
						.toActionId = ActionId::Knight_Hit,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 100
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnParried,
						.toActionId = ActionId::Knight_Stun,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 200
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Knight_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules = {}
			},
			.combatWindows =
			{
				ActionCombatWindowDef
				{
					.windowType = CombatWindowType::Attack,
					.startNormalized = 0.3f,
					.endNormalized = 1.0f,
					.appliesTo = ActionCombatApplyTo::FrontPhysical,
					.spatialFilter = std::nullopt,
					.effect = CombatEffectDef
					{
						.type = CombatEffectType::AttackHit,
						.attackHit = AttackCombatEffectDef
						{
							.damageScale = 3.5f,
							.bonusDamage = 13.0f,
							.staminaDamageScale = 2.2f,
							.bonusStaminaDamage = 6.0f,
							.poiseDamageScale = 0.0f,
							.bonusPoiseDamage = 0.0f,
							.knockbackDistance = 1.40f,
							.hitStopSec = 0.11f,
							.parryable = true,
							.guardable = true
						},
						.parryResponse = std::nullopt,
						.guardResponse = std::nullopt
					}
				}
			},
			.events = {},
			.moveSegments = {}
		},

		ActionDef
		{
			.id = ActionId::Knight_Dodge,
			.name = "Knight.Dodge",
			.characterId = CharacterId::Knight,
			.kind = ActionKind::Dodge,
			.playerInput = PlayerActionInput::None,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = 1.53f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements =
			{
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::HasEnoughStamina,
					.scalar = 0,
					.stateFlag = std::nullopt
				},
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::IsGrounded,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				}
			},
			.resourceCosts =
			{
				ActionResourceCostDef
				{
					.type = ActionResourceType::Stamina,
					.consumeTiming = ActionResourceConsumeTiming::OnRequest,
					.amount = 0
				}
			},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Knight_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules = {}
			},
			.combatWindows =
			{
				ActionCombatWindowDef
				{
					.windowType = CombatWindowType::Invulnerability,
					.startNormalized = 0.08f,
					.endNormalized = 0.92f,
					.appliesTo = std::nullopt,
					.spatialFilter = std::nullopt,
					.effect = std::nullopt,
				}
			},
			.events = {},
			.moveSegments =
			{
				ActionMovementSegmentDef
				{
					.startNormalized = 0 / 50.f,
					.endNormalized = 7 / 50.f,
					.horizontalMoveMode = HorizontalMovementMode::InputDirectionDistance,
					.moveDistance = 22.796f / 100.0f,
					.rotationMode = RotationMode::None,
					.rotationRate = std::nullopt,
					.verticalMoveMode = VerticalMovementMode::None,
					.verticalAmount = std::nullopt,
					.dirPolicy = DirectionPolicy::ActionStartInput,
					.dirSampleTiming = DirectionSampleTiming::OnSegmentStart
				},
				ActionMovementSegmentDef
				{
					.startNormalized = 7 / 50.f,
					.endNormalized = 40 / 50.f,
					.horizontalMoveMode = HorizontalMovementMode::InputDirectionDistance,
					.moveDistance = 319.144 / 100.0f,
					.rotationMode = RotationMode::None,
					.rotationRate = std::nullopt,
					.verticalMoveMode = VerticalMovementMode::None,
					.verticalAmount = std::nullopt,
					.dirPolicy = DirectionPolicy::LockedDirection,
					.dirSampleTiming = DirectionSampleTiming::OnSegmentStart
				},
				ActionMovementSegmentDef
				{
					.startNormalized = 40 / 50.f,
					.endNormalized = 50 / 50.f,
					.horizontalMoveMode = HorizontalMovementMode::InputDirectionDistance,
					.moveDistance = 6.03 / 100.f,
					.rotationMode = RotationMode::None,
					.rotationRate = std::nullopt,
					.verticalMoveMode = VerticalMovementMode::None,
					.verticalAmount = std::nullopt,
					.dirPolicy = DirectionPolicy::LockedDirection,
					.dirSampleTiming = DirectionSampleTiming::OnSegmentStart
				}
			}
		},

		ActionDef
		{
			.id = ActionId::Knight_Parry,
			.name = "Knight.Parry",
			.characterId = CharacterId::Knight,
			.kind = ActionKind::Parry,
			.playerInput = PlayerActionInput::Parry,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = 1.00f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements =
			{
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::HasEnoughStamina,
					.scalar = 0,
					.stateFlag = std::nullopt
				},
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::IsGrounded,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				}
			},
			.resourceCosts =
			{
				ActionResourceCostDef
				{
					.type = ActionResourceType::Stamina,
					.consumeTiming = ActionResourceConsumeTiming::OnRequest,
					.amount = 0
				}
			},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHitReceived,
						.toActionId = ActionId::Knight_Hit,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 100
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Knight_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules = {}
			},
			.combatWindows =
			{
				ActionCombatWindowDef
				{
					.windowType = CombatWindowType::Parry,
					.startNormalized = 0.3f,
					.endNormalized = 1.0f,
					.appliesTo = ActionCombatApplyTo::ParryableAttack,
					.spatialFilter = ActionCombatSpatialFilterDef
					{
						.facingHalfAngleDeg = 45.0f,
						.minDistance = std::nullopt,
						.maxDistance = 2.0f,
						.verticalTolerance = 1.0f,
						.referenceFrame = CombatReferenceFrame::OwnerFacing
					},
					.effect = CombatEffectDef
					{
						.type = CombatEffectType::ParryResponse,
						.attackHit = std::nullopt,
						.parryResponse = ParryCombatEffectDef
						{
							.stunSec = 0.85f,
							.hitStopSec = 0.08f,
							.grantBuffId = BuffId::ParrySuccess
						},
						.guardResponse = std::nullopt
					}
				}
			},
			.events = {},
			.moveSegments = {}
		},

		ActionDef
		{
			.id = ActionId::Knight_Guard,
			.name = "Knight.Guard",
			.characterId = CharacterId::Knight,
			.kind = ActionKind::Guard,
			.playerInput = PlayerActionInput::Guard,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = (std::numeric_limits<float>::max)(),
			.normalizedPolicy = ActionNormalizedPolicy::Holdable,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::HoldRelease,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements =
			{
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::HasEnoughStamina,
					.scalar = 0,
					.stateFlag = std::nullopt
				},
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::IsGrounded,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				}
			},
			.resourceCosts =
			{
				ActionResourceCostDef
				{
					.type = ActionResourceType::Stamina,
					.consumeTiming = ActionResourceConsumeTiming::OnRequest,
					.amount = 0
				}
			},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHitReceived,
						.toActionId = ActionId::Knight_Hit,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 100
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Knight_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules = 
				{
					ActionCancelRule
					{
						.cancelKind = ActionCancelKind::HoldRelease,
						.toActionId = ActionId::None,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 10
					},
					ActionCancelRule
					{
						.cancelKind = ActionCancelKind::LightAttackCancel,
						.toActionId = ActionId::Knight_LightAttack1,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 100
					},
					ActionCancelRule
					{
						.cancelKind = ActionCancelKind::HeavyAttackCancel,
						.toActionId = ActionId::Knight_HeavyAttack,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 110
					},
					ActionCancelRule
					{
						.cancelKind = ActionCancelKind::DodgeCancel,
						.toActionId = ActionId::Knight_Dodge,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 200
					},
					ActionCancelRule
					{
						.cancelKind = ActionCancelKind::ParryCancel,
						.toActionId = ActionId::Knight_Parry,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 300
					}
				}
			},
			.combatWindows =
			{
				ActionCombatWindowDef
				{
					.windowType = CombatWindowType::Guard,
					.startNormalized = 0.05f,
					.endNormalized = 1.0f,
					.appliesTo = ActionCombatApplyTo::GuardableAttack,
					.spatialFilter = ActionCombatSpatialFilterDef
					{
						.facingHalfAngleDeg = 60.0f,
						.minDistance = std::nullopt,
						.maxDistance = std::nullopt,
						.verticalTolerance = std::nullopt,
						.referenceFrame = CombatReferenceFrame::OwnerFacing
					},
					.effect = CombatEffectDef
					{
						.type = CombatEffectType::GuardResponse,
						.attackHit = std::nullopt,
						.parryResponse = std::nullopt,
						.guardResponse = GuardCombatEffectDef
						{
							.damageReductionRatio = 0.85f,
							.chipDamageRatio = 0.10f,
							.staminaDamageMultiplier = 1.20f,
							.hitStopSec = 0.04f
						}
					}
				}
			},
			.events = {},
			.moveSegments = {}
		},

		ActionDef
		{
			.id = ActionId::Knight_Stun,
			.name = "Knight.Stun",
			.characterId = CharacterId::Knight,
			.kind = ActionKind::Stun,
			.playerInput = PlayerActionInput::None,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = 3.16f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements = {},
			.resourceCosts = {},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Knight_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules = {}
			},
			.combatWindows = {},
			.events = {},
			.moveSegments = {}
		},

		ActionDef
		{
			.id = ActionId::Knight_Hit,
			.name = "Knight.Hit",
			.characterId = CharacterId::Knight,
			.kind = ActionKind::Hit,
			.playerInput = PlayerActionInput::None,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = 1.63f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements = {},
			.resourceCosts = {},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Knight_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules = {}
			},
			.combatWindows = {},
			.events = {},
			.moveSegments = {}
		},

		ActionDef
		{
			.id = ActionId::Knight_Dead,
			.name = "Knight.Dead",
			.characterId = CharacterId::Knight,
			.kind = ActionKind::Dead,
			.playerInput = PlayerActionInput::None,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = 3.86f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements = {},
			.resourceCosts = {},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules = {},
				.cancelRules = {}
			},
			.combatWindows = {},
			.events = {},
			.moveSegments = {}
		},

		ActionDef
		{
			.id = ActionId::Imp_melee1,
			.name = "Imp.Melee1",
			.characterId = CharacterId::Imp,
			.kind = ActionKind::Attack,
			.playerInput = PlayerActionInput::None,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = 0.82f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements =
			{
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::HasTarget,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				},
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::IsGrounded,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				}
			},
			.resourceCosts = {},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHitReceived,
						.toActionId = ActionId::Imp_Hit,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 100
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnParried,
						.toActionId = ActionId::Imp_Stun,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 200
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Imp_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules = {}
			},
			.combatWindows =
			{
				ActionCombatWindowDef
				{
					.windowType = CombatWindowType::Attack,
					.startNormalized = 0.30f,
					.endNormalized = 0.52f,
					.appliesTo = ActionCombatApplyTo::FrontPhysical,
					.spatialFilter = std::nullopt,
					.effect = CombatEffectDef
					{
						.type = CombatEffectType::AttackHit,
						.attackHit = AttackCombatEffectDef
						{
							.damageScale = 0.6f,
							.bonusDamage = 2.0f,
							.staminaDamageScale = 0.4f,
							.bonusStaminaDamage = 2.0f,
							.poiseDamageScale = 0.0f,
							.bonusPoiseDamage = 0.0f,
							.knockbackDistance = 0.20f,
							.hitStopSec = 0.04f,
							.parryable = true,
							.guardable = true
						},
						.parryResponse = std::nullopt,
						.guardResponse = std::nullopt
					}
				}
			},
			.events = {},
			.moveSegments =
			{
				ActionMovementSegmentDef
				{
					.startNormalized = 0.10f,
					.endNormalized = 0.36f,
					.horizontalMoveMode = HorizontalMovementMode::ForwardFixedDistance,
					.moveDistance = 0.55f,
					.rotationMode = RotationMode::FaceTarget,
					.rotationRate = std::nullopt,
					.verticalMoveMode = VerticalMovementMode::None,
					.verticalAmount = std::nullopt,
					.dirPolicy = DirectionPolicy::TargetDirection,
					.dirSampleTiming = DirectionSampleTiming::OnSegmentStart
				}
			}
		},

		ActionDef
		{
			.id = ActionId::Imp_melee2,
			.name = "Imp.Melee2",
			.characterId = CharacterId::Imp,
			.kind = ActionKind::Attack,
			.playerInput = PlayerActionInput::None,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = 0.88f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements =
			{
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::HasTarget,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				},
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::IsGrounded,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				}
			},
			.resourceCosts = {},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHitReceived,
						.toActionId = ActionId::Imp_Hit,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 100
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnParried,
						.toActionId = ActionId::Imp_Stun,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 200
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Imp_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules = {}
			},
			.combatWindows =
			{
				ActionCombatWindowDef
				{
					.windowType = CombatWindowType::Attack,
					.startNormalized = 0.34f,
					.endNormalized = 0.58f,
					.appliesTo = ActionCombatApplyTo::FrontPhysical,
					.spatialFilter = std::nullopt,
					.effect = CombatEffectDef
					{
						.type = CombatEffectType::AttackHit,
						.attackHit = AttackCombatEffectDef
						{
							.damageScale = 0.7f,
							.bonusDamage = 3.0f,
							.staminaDamageScale = 0.5f,
							.bonusStaminaDamage = 2.0f,
							.poiseDamageScale = 0.0f,
							.bonusPoiseDamage = 0.0f,
							.knockbackDistance = 0.25f,
							.hitStopSec = 0.04f,
							.parryable = true,
							.guardable = true
						},
						.parryResponse = std::nullopt,
						.guardResponse = std::nullopt
					}
				}
			},
			.events = {},
			.moveSegments =
			{
				ActionMovementSegmentDef
				{
					.startNormalized = 0.12f,
					.endNormalized = 0.38f,
					.horizontalMoveMode = HorizontalMovementMode::ForwardFixedDistance,
					.moveDistance = 0.65f,
					.rotationMode = RotationMode::FaceTarget,
					.rotationRate = std::nullopt,
					.verticalMoveMode = VerticalMovementMode::None,
					.verticalAmount = std::nullopt,
					.dirPolicy = DirectionPolicy::TargetDirection,
					.dirSampleTiming = DirectionSampleTiming::OnSegmentStart
				}
			}
		},

		ActionDef
		{
			.id = ActionId::Imp_melee3,
			.name = "Imp.Melee3",
			.characterId = CharacterId::Imp,
			.kind = ActionKind::Attack,
			.playerInput = PlayerActionInput::None,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = 0.94f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements =
			{
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::HasTarget,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				},
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::IsGrounded,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				}
			},
			.resourceCosts = {},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHitReceived,
						.toActionId = ActionId::Imp_Hit,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 100
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnParried,
						.toActionId = ActionId::Imp_Stun,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 200
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Imp_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules = {}
			},
			.combatWindows =
			{
				ActionCombatWindowDef
				{
					.windowType = CombatWindowType::Attack,
					.startNormalized = 0.38f,
					.endNormalized = 0.62f,
					.appliesTo = ActionCombatApplyTo::FrontPhysical,
					.spatialFilter = std::nullopt,
					.effect = CombatEffectDef
					{
						.type = CombatEffectType::AttackHit,
						.attackHit = AttackCombatEffectDef
						{
							.damageScale = 0.8f,
							.bonusDamage = 4.0f,
							.staminaDamageScale = 0.6f,
							.bonusStaminaDamage = 2.0f,
							.poiseDamageScale = 0.0f,
							.bonusPoiseDamage = 0.0f,
							.knockbackDistance = 0.30f,
							.hitStopSec = 0.05f,
							.parryable = true,
							.guardable = true
						},
						.parryResponse = std::nullopt,
						.guardResponse = std::nullopt
					}
				}
			},
			.events = {},
			.moveSegments =
			{
				ActionMovementSegmentDef
				{
					.startNormalized = 0.14f,
					.endNormalized = 0.40f,
					.horizontalMoveMode = HorizontalMovementMode::ForwardFixedDistance,
					.moveDistance = 0.72f,
					.rotationMode = RotationMode::FaceTarget,
					.rotationRate = std::nullopt,
					.verticalMoveMode = VerticalMovementMode::None,
					.verticalAmount = std::nullopt,
					.dirPolicy = DirectionPolicy::TargetDirection,
					.dirSampleTiming = DirectionSampleTiming::OnSegmentStart
				}
			}
		},

		ActionDef
		{
			.id = ActionId::Imp_melee4,
			.name = "Imp.Melee4",
			.characterId = CharacterId::Imp,
			.kind = ActionKind::Attack,
			.playerInput = PlayerActionInput::None,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = 1.02f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements =
			{
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::HasTarget,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				},
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::IsGrounded,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				}
			},
			.resourceCosts = {},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHitReceived,
						.toActionId = ActionId::Imp_Hit,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 100
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnParried,
						.toActionId = ActionId::Imp_Stun,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 200
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Imp_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules = {}
			},
			.combatWindows =
			{
				ActionCombatWindowDef
				{
					.windowType = CombatWindowType::Attack,
					.startNormalized = 0.42f,
					.endNormalized = 0.68f,
					.appliesTo = ActionCombatApplyTo::FrontPhysical,
					.spatialFilter = std::nullopt,
					.effect = CombatEffectDef
					{
						.type = CombatEffectType::AttackHit,
						.attackHit = AttackCombatEffectDef
						{
							.damageScale = 0.9f,
							.bonusDamage = 5.0f,
							.staminaDamageScale = 0.8f,
							.bonusStaminaDamage = 2.0f,
							.poiseDamageScale = 0.0f,
							.bonusPoiseDamage = 0.0f,
							.knockbackDistance = 0.38f,
							.hitStopSec = 0.05f,
							.parryable = true,
							.guardable = true
						},
						.parryResponse = std::nullopt,
						.guardResponse = std::nullopt
					}
				}
			},
			.events = {},
			.moveSegments =
			{
				ActionMovementSegmentDef
				{
					.startNormalized = 0.16f,
					.endNormalized = 0.42f,
					.horizontalMoveMode = HorizontalMovementMode::ForwardFixedDistance,
					.moveDistance = 0.85f,
					.rotationMode = RotationMode::FaceTarget,
					.rotationRate = std::nullopt,
					.verticalMoveMode = VerticalMovementMode::None,
					.verticalAmount = std::nullopt,
					.dirPolicy = DirectionPolicy::TargetDirection,
					.dirSampleTiming = DirectionSampleTiming::OnSegmentStart
				}
			}
		},

		ActionDef
		{
			.id = ActionId::Imp_melee5,
			.name = "Imp.Melee5",
			.characterId = CharacterId::Imp,
			.kind = ActionKind::Attack,
			.playerInput = PlayerActionInput::None,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = 1.10f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements =
			{
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::HasTarget,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				},
				ActionRequestRequirementDef
				{
					.type = ActionRequestRequirementType::IsGrounded,
					.scalar = std::nullopt,
					.stateFlag = std::nullopt
				}
			},
			.resourceCosts = {},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHitReceived,
						.toActionId = ActionId::Imp_Hit,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 100
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnParried,
						.toActionId = ActionId::Imp_Stun,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 200
					},
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Imp_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules = {}
			},
			.combatWindows =
			{
				ActionCombatWindowDef
				{
					.windowType = CombatWindowType::Attack,
					.startNormalized = 0.46f,
					.endNormalized = 0.74f,
					.appliesTo = ActionCombatApplyTo::FrontPhysical,
					.spatialFilter = std::nullopt,
					.effect = CombatEffectDef
					{
						.type = CombatEffectType::AttackHit,
						.attackHit = AttackCombatEffectDef
						{
							.damageScale = 1.1f,
							.bonusDamage = 7.0f,
							.staminaDamageScale = 1.0f,
							.bonusStaminaDamage = 2.0f,
							.poiseDamageScale = 0.0f,
							.bonusPoiseDamage = 0.0f,
							.knockbackDistance = 0.55f,
							.hitStopSec = 0.06f,
							.parryable = true,
							.guardable = true
						},
						.parryResponse = std::nullopt,
						.guardResponse = std::nullopt
					}
				}
			},
			.events = {},
			.moveSegments =
			{
				ActionMovementSegmentDef
				{
					.startNormalized = 0.18f,
					.endNormalized = 0.46f,
					.horizontalMoveMode = HorizontalMovementMode::ForwardFixedDistance,
					.moveDistance = 0.95f,
					.rotationMode = RotationMode::FaceTarget,
					.rotationRate = std::nullopt,
					.verticalMoveMode = VerticalMovementMode::None,
					.verticalAmount = std::nullopt,
					.dirPolicy = DirectionPolicy::TargetDirection,
					.dirSampleTiming = DirectionSampleTiming::OnSegmentStart
				}
			}
		},

		ActionDef
		{
			.id = ActionId::Imp_Stun,
			.name = "Imp.Stun",
			.characterId = CharacterId::Imp,
			.kind = ActionKind::Stun,
			.playerInput = PlayerActionInput::None,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = 1.00f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements = {},
			.resourceCosts = {},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Imp_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules = {}
			},
			.combatWindows = {},
			.events = {},
			.moveSegments = {}
		},

		ActionDef
		{
			.id = ActionId::Imp_Hit,
			.name = "Imp.Hit",
			.characterId = CharacterId::Imp,
			.kind = ActionKind::Hit,
			.playerInput = PlayerActionInput::None,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = 0.58f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements = {},
			.resourceCosts = {},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules =
				{
					ActionInterruptRule
					{
						.causeType = ActionInterruptCauseType::OnHpZero,
						.toActionId = ActionId::Imp_Dead,
						.windowPolicy = ActionWindowPolicy::Always,
						.windowStartNormalized = std::nullopt,
						.windowEndNormalized = std::nullopt,
						.priority = 1000
					}
				},
				.cancelRules = {}
			},
			.combatWindows = {},
			.events = {},
			.moveSegments = {}
		},

		ActionDef
		{
			.id = ActionId::Imp_Dead,
			.name = "Imp.Dead",
			.characterId = CharacterId::Imp,
			.kind = ActionKind::Dead,
			.playerInput = PlayerActionInput::None,
			.comboGroupId = std::nullopt,
			.comboIndex = std::nullopt,
			.duration = 1.10f,
			.normalizedPolicy = ActionNormalizedPolicy::FixedDuration,
			.endPolicy = ActionEndPolicyDef
			{
				.endType = ActionEndType::NaturalEnd,
				.defaultNextActionId = ActionId::None
			},
			.requestRequirements = {},
			.resourceCosts = {},
			.transitionRule = ActionTransitionRuleDef
			{
				.interruptRules = {},
				.cancelRules = {}
			},
			.combatWindows = {},
			.events = {},
			.moveSegments = {}
		}
	};


}

const ActionDef* FindActionDef(ActionId id) noexcept
{
	for (const ActionDef& def : kActionDefs)
	{
		if (def.id == id)
		{
			return &def;
		}
	}

	return nullptr;
}

const ActionDef& GetActionDef(ActionId id)
{
	const ActionDef* const def = FindActionDef(id);
	if (def == nullptr)
	{
		throw std::out_of_range("ActionDef was not found.");
	}

	return *def;
}

std::span<const ActionDef> GetActionDefs() noexcept
{
	return std::span<const ActionDef>(kActionDefs);
}
