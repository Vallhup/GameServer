#include "pch.h"
#include "ActionDef.h"
#include "ECS/GameplayRuntimeComponents.h"

#include <array>
#include <stdexcept>

namespace
{
	const std::array<ActionDef, 20> kActionDefs =
	{
		ActionDef
		{
			.id = ActionId::Knight_LightAttack1,
			.name = "Knight.LightAttack1",
			.kind = ActionKind::Attack,
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
			.kind = ActionKind::Attack,
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
			.kind = ActionKind::Attack,
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
			.kind = ActionKind::Attack,
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
			.kind = ActionKind::Attack,
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
			.kind = ActionKind::Dodge,
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
			.kind = ActionKind::Parry,
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
			.kind = ActionKind::Guard,
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
			.kind = ActionKind::Stun,
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
			.kind = ActionKind::Hit,
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
			.kind = ActionKind::Dead,
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
			.kind = ActionKind::Attack,
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
					.endNormalized = 0.74f,
					.appliesTo = ActionCombatApplyTo::FrontPhysical,
					.spatialFilter = ActionCombatSpatialFilterDef
					{
						.facingHalfAngleDeg = 52.0f,
						.minDistance = std::nullopt,
						.maxDistance = 2.20f,
						.verticalTolerance = 1.20f,
						.referenceFrame = CombatReferenceFrame::LockedActionDirection
					},
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
					.horizontalMoveMode = HorizontalMovementMode::None,
					.moveDistance = std::nullopt,
					.rotationMode = RotationMode::FaceTarget,
					.rotationRate = 5.5f,
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
			.kind = ActionKind::Attack,
			.duration = 2.63f,
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
					.endNormalized = 0.68f,
					.appliesTo = ActionCombatApplyTo::FrontPhysical,
					.spatialFilter = ActionCombatSpatialFilterDef
					{
						.facingHalfAngleDeg = 62.0f,
						.minDistance = std::nullopt,
						.maxDistance = 2.35f,
						.verticalTolerance = 1.20f,
						.referenceFrame = CombatReferenceFrame::LockedActionDirection
					},
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
					.horizontalMoveMode = HorizontalMovementMode::None,
					.moveDistance = std::nullopt,
					.rotationMode = RotationMode::FaceTarget,
					.rotationRate = 4.5f,
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
			.kind = ActionKind::Attack,
			.duration = 2.23f,
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
					.startNormalized = 0.26f,
					.endNormalized = 0.46f,
					.appliesTo = ActionCombatApplyTo::FrontPhysical,
					.spatialFilter = ActionCombatSpatialFilterDef
					{
						.facingHalfAngleDeg = 58.0f,
						.minDistance = std::nullopt,
						.maxDistance = 2.30f,
						.verticalTolerance = 1.20f,
						.referenceFrame = CombatReferenceFrame::LockedActionDirection
					},
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
					.horizontalMoveMode = HorizontalMovementMode::None,
					.moveDistance = std::nullopt,
					.rotationMode = RotationMode::FaceTarget,
					.rotationRate = 5.0f,
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
			.kind = ActionKind::Attack,
			.duration = 2.36f,
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
					.startNormalized = 0.22f,
					.endNormalized = 0.84f,
					.appliesTo = ActionCombatApplyTo::FrontPhysical,
					.spatialFilter = ActionCombatSpatialFilterDef
					{
						.facingHalfAngleDeg = 72.0f,
						.minDistance = std::nullopt,
						.maxDistance = 2.50f,
						.verticalTolerance = 1.20f,
						.referenceFrame = CombatReferenceFrame::LockedActionDirection
					},
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
					.horizontalMoveMode = HorizontalMovementMode::None,
					.moveDistance = std::nullopt,
					.rotationMode = RotationMode::FaceTarget,
					.rotationRate = 4.0f,
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
			.kind = ActionKind::Attack,
			.duration = 2.60f,
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
					.startNormalized = 0.28f,
					.endNormalized = 0.66f,
					.appliesTo = ActionCombatApplyTo::FrontPhysical,
					.spatialFilter = ActionCombatSpatialFilterDef
					{
						.facingHalfAngleDeg = 68.0f,
						.minDistance = std::nullopt,
						.maxDistance = 2.60f,
						.verticalTolerance = 1.20f,
						.referenceFrame = CombatReferenceFrame::LockedActionDirection
					},
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
					.horizontalMoveMode = HorizontalMovementMode::None,
					.moveDistance = std::nullopt,
					.rotationMode = RotationMode::FaceTarget,
					.rotationRate = 3.5f,
					.verticalMoveMode = VerticalMovementMode::None,
					.verticalAmount = std::nullopt,
					.dirPolicy = DirectionPolicy::TargetDirection,
					.dirSampleTiming = DirectionSampleTiming::OnSegmentStart
				}
			}
		},

		ActionDef
		{
			.id = ActionId::Imp_Jump,
			.name = "Imp.Jump",
			.kind = ActionKind::NonCombat,
			.duration = 2.333331f,
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
			.id = ActionId::Imp_Stun,
			.name = "Imp.Stun",
			.kind = ActionKind::Stun,
			.duration = 2.0f,
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
			.kind = ActionKind::Hit,
			.duration = 1.16f,
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
			.kind = ActionKind::Dead,
			.duration = 2.33f,
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

	const std::array<CharacterActionProfileDef, 3> kCharacterActionProfiles =
	{
		CharacterActionProfileDef
		{
			.id = CharacterActionProfileIds::Knight,
			.characterId = CharacterId::Knight,
			.availableActions =
			{
				ActionId::Knight_LightAttack1,
				ActionId::Knight_LightAttack2,
				ActionId::Knight_LightAttack3,
				ActionId::Knight_HeavyAttack,
				ActionId::Knight_Dodge,
				ActionId::Knight_Parry,
				ActionId::Knight_Stun,
				ActionId::Knight_Hit,
				ActionId::Knight_Guard,
				ActionId::Knight_UseHpPotion,
				ActionId::Knight_Dead
			},
			.inputBindingProfileId = ActionInputBindingProfileIds::Knight,
			.fallbackReactionProfileId = ActionFallbackReactionProfileIds::Knight,
			.animationBindingProfileId = AnimationBindingProfileIds::Knight
		},
		CharacterActionProfileDef
		{
			.id = CharacterActionProfileIds::Imp,
			.characterId = CharacterId::Imp,
			.availableActions =
			{
				ActionId::Imp_melee1,
				ActionId::Imp_melee3,
				ActionId::Imp_melee4,
				ActionId::Imp_melee5,
				ActionId::Imp_Jump,
				ActionId::Imp_Stun,
				ActionId::Imp_Hit,
				ActionId::Imp_Dead
			},
			.inputBindingProfileId = ActionInputBindingProfileIds::Imp,
			.fallbackReactionProfileId = ActionFallbackReactionProfileIds::Imp,
			.animationBindingProfileId = AnimationBindingProfileIds::Imp
		},
		CharacterActionProfileDef
		{
			.id = CharacterActionProfileIds::FinalBoss,
			.characterId = CharacterId::FinalBoss,
			.availableActions =
			{
				ActionId::FinalBoss_Thrust,
				ActionId::FinalBoss_Slash,
				ActionId::FinalBoss_DashSlash,
				ActionId::FinalBoss_JumpSlash,
				ActionId::FinalBoss_MultiSlash,
				ActionId::FinalBoss_Stun,
				ActionId::FinalBoss_Hit,
				ActionId::FinalBoss_Dead
			},
			.inputBindingProfileId = ActionInputBindingProfileIds::FinalBoss,
			.fallbackReactionProfileId = ActionFallbackReactionProfileIds::FinalBoss,
			.animationBindingProfileId = AnimationBindingProfileIds::FinalBoss
		}
	};

	const std::array<ActionInputBindingProfileDef, 3> kActionInputBindingProfiles =
	{
		ActionInputBindingProfileDef
		{
			.id = ActionInputBindingProfileIds::Knight,
			.entries =
			{
				ActionInputBindingEntryDef
				{
					.request = ActionRequestSemantic::LightAttack,
					.candidateActions =
					{
						ActionId::Knight_LightAttack1,
						ActionId::Knight_LightAttack2,
						ActionId::Knight_LightAttack3
					},
					.selectionPolicy = ActionCandidateSelectionPolicy::OrderedFirstValid,
					.priority = 100
				},
				ActionInputBindingEntryDef
				{
					.request = ActionRequestSemantic::HeavyAttack,
					.candidateActions = { ActionId::Knight_HeavyAttack },
					.selectionPolicy = ActionCandidateSelectionPolicy::OrderedFirstValid,
					.priority = 100
				},
				ActionInputBindingEntryDef
				{
					.request = ActionRequestSemantic::Dodge,
					.candidateActions = { ActionId::Knight_Dodge },
					.selectionPolicy = ActionCandidateSelectionPolicy::OrderedFirstValid,
					.priority = 100
				},
				ActionInputBindingEntryDef
				{
					.request = ActionRequestSemantic::Parry,
					.candidateActions = { ActionId::Knight_Parry },
					.selectionPolicy = ActionCandidateSelectionPolicy::OrderedFirstValid,
					.priority = 100
				},
				ActionInputBindingEntryDef
				{
					.request = ActionRequestSemantic::GuardStart,
					.candidateActions = { ActionId::Knight_Guard },
					.selectionPolicy = ActionCandidateSelectionPolicy::OrderedFirstValid,
					.priority = 100
				}
			}
		},
		ActionInputBindingProfileDef
		{
			.id = ActionInputBindingProfileIds::Imp,
			.entries = {}
		},
		ActionInputBindingProfileDef
		{
			.id = ActionInputBindingProfileIds::FinalBoss,
			.entries = {}
		}
	};

	const std::array<ActionFallbackReactionProfileDef, 3> kActionFallbackReactionProfiles =
	{
		ActionFallbackReactionProfileDef
		{
			.id = ActionFallbackReactionProfileIds::Knight,
			.entries =
			{
				ActionFallbackReactionEntryDef
				{
					.causeType = ActionInterruptCauseType::OnHitReceived,
					.toActionId = ActionId::Knight_Hit,
					.priority = 100
				},
				ActionFallbackReactionEntryDef
				{
					.causeType = ActionInterruptCauseType::OnParried,
					.toActionId = ActionId::Knight_Stun,
					.priority = 200
				},
				ActionFallbackReactionEntryDef
				{
					.causeType = ActionInterruptCauseType::OnHpZero,
					.toActionId = ActionId::Knight_Dead,
					.priority = 1000
				}
			}
		},
		ActionFallbackReactionProfileDef
		{
			.id = ActionFallbackReactionProfileIds::Imp,
			.entries =
			{
				ActionFallbackReactionEntryDef
				{
					.causeType = ActionInterruptCauseType::OnHitReceived,
					.toActionId = ActionId::Imp_Hit,
					.priority = 100
				},
				ActionFallbackReactionEntryDef
				{
					.causeType = ActionInterruptCauseType::OnParried,
					.toActionId = ActionId::Imp_Stun,
					.priority = 200
				},
				ActionFallbackReactionEntryDef
				{
					.causeType = ActionInterruptCauseType::OnHpZero,
					.toActionId = ActionId::Imp_Dead,
					.priority = 1000
				}
			}
		},
		ActionFallbackReactionProfileDef
		{
			.id = ActionFallbackReactionProfileIds::FinalBoss,
			.entries =
			{
				ActionFallbackReactionEntryDef
				{
					.causeType = ActionInterruptCauseType::OnHitReceived,
					.toActionId = ActionId::FinalBoss_Hit,
					.priority = 100
				},
				ActionFallbackReactionEntryDef
				{
					.causeType = ActionInterruptCauseType::OnParried,
					.toActionId = ActionId::FinalBoss_Stun,
					.priority = 200
				},
				ActionFallbackReactionEntryDef
				{
					.causeType = ActionInterruptCauseType::OnHpZero,
					.toActionId = ActionId::FinalBoss_Dead,
					.priority = 1000
				}
			}
		}
	};

	const std::array<AnimationBindingProfileDef, 3> kAnimationBindingProfiles =
	{
		AnimationBindingProfileDef
		{
			.id = AnimationBindingProfileIds::Knight,
			.actionBindings =
			{
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Knight_LightAttack1,
					.animationId = AnimationId::Knight_LightAttack1
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Knight_Dodge,
					.animationId = AnimationId::Knight_Dodge
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Knight_Parry,
					.animationId = AnimationId::Knight_Parry
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Knight_Stun,
					.animationId = AnimationId::Knight_Stun
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Knight_Hit,
					.animationId = AnimationId::Knight_Hit
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Knight_Guard,
					.animationId = AnimationId::Knight_Guard
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Knight_UseHpPotion,
					.animationId = AnimationId::Knight_Drinking
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Knight_Dead,
					.animationId = AnimationId::Knight_Death
				}
			},
			.locomotionBindings =
			{
				LocomotionAnimationBindingDef
				{
					.mode = LocomotionMode::Idle,
					.animationId = AnimationId::Knight_Idle,
					.holdLastFrame = true
				},
				LocomotionAnimationBindingDef
				{
					.mode = LocomotionMode::Walk,
					.animationId = AnimationId::Knight_Walk
				},
				LocomotionAnimationBindingDef
				{
					.mode = LocomotionMode::Run,
					.animationId = AnimationId::Knight_Run
				},
				LocomotionAnimationBindingDef
				{
					.mode = LocomotionMode::Turn,
					.animationId = AnimationId::Knight_Walk
				}
			}
		},
		AnimationBindingProfileDef
		{
			.id = AnimationBindingProfileIds::Imp,
			.actionBindings =
			{
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Imp_melee1,
					.animationId = AnimationId::Imp_Melee_1
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Imp_melee2,
					.animationId = AnimationId::Imp_Melee_2
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Imp_melee3,
					.animationId = AnimationId::Imp_Melee_3
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Imp_melee4,
					.animationId = AnimationId::Imp_Melee_4
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Imp_melee5,
					.animationId = AnimationId::Imp_Melee_5
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Imp_Jump,
					.animationId = AnimationId::Imp_Jump_1
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Imp_Hit,
					.animationId = AnimationId::Imp_React_Front
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Imp_Stun,
					.animationId = AnimationId::Imp_Stun
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::Imp_Dead,
					.animationId = AnimationId::Imp_Death_1
				}
			},
			.locomotionBindings =
			{
				LocomotionAnimationBindingDef
				{
					.mode = LocomotionMode::Idle,
					.animationId = AnimationId::Imp_Idle_1,
					.holdLastFrame = true
				},
				LocomotionAnimationBindingDef
				{
					.mode = LocomotionMode::Walk,
					.animationId = AnimationId::Imp_Walk_Forward
				},
				LocomotionAnimationBindingDef
				{
					.mode = LocomotionMode::Run,
					.animationId = AnimationId::Imp_Walk_Forward
				},
				LocomotionAnimationBindingDef
				{
					.mode = LocomotionMode::Turn,
					.animationId = AnimationId::Imp_Walk_Forward
				}
			}
		},
		AnimationBindingProfileDef
		{
			.id = AnimationBindingProfileIds::FinalBoss,
			.actionBindings =
			{
				ActionAnimationBindingDef
				{
					.actionId = ActionId::FinalBoss_Thrust,
					.animationId = AnimationId::FinalBoss_Thrust
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::FinalBoss_Slash,
					.animationId = AnimationId::FinalBoss_Slash
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::FinalBoss_DashSlash,
					.animationId = AnimationId::FinalBoss_DashSlash
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::FinalBoss_JumpSlash,
					.animationId = AnimationId::FinalBoss_JumpSlash
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::FinalBoss_MultiSlash,
					.animationId = AnimationId::FinalBoss_MultiSlash
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::FinalBoss_Stun,
					.animationId = AnimationId::FinalBoss_Stun
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::FinalBoss_Hit,
					.animationId = AnimationId::FinalBoss_Hit
				},
				ActionAnimationBindingDef
				{
					.actionId = ActionId::FinalBoss_Dead,
					.animationId = AnimationId::FinalBoss_Death
				}
			},
			.locomotionBindings =
			{
				LocomotionAnimationBindingDef
				{
					.mode = LocomotionMode::Idle,
					.animationId = AnimationId::FinalBoss_Idle,
					.holdLastFrame = true
				},
				LocomotionAnimationBindingDef
				{
					.mode = LocomotionMode::Walk,
					.animationId = AnimationId::FinalBoss_Walk
				},
				LocomotionAnimationBindingDef
				{
					.mode = LocomotionMode::Run,
					.animationId = AnimationId::FinalBoss_Walk
				},
				LocomotionAnimationBindingDef
				{
					.mode = LocomotionMode::Turn,
					.animationId = AnimationId::FinalBoss_Walk
				}
			}
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

const CharacterActionProfileDef* FindCharacterActionProfileDef(
	CharacterActionProfileId id) noexcept
{
	for (const CharacterActionProfileDef& def : kCharacterActionProfiles)
	{
		if (def.id == id)
		{
			return &def;
		}
	}

	return nullptr;
}

const CharacterActionProfileDef& GetCharacterActionProfileDef(
	CharacterActionProfileId id)
{
	const CharacterActionProfileDef* const def = FindCharacterActionProfileDef(id);
	if (def == nullptr)
	{
		throw std::out_of_range("CharacterActionProfileDef was not found.");
	}

	return *def;
}

const CharacterActionProfileDef* FindCharacterActionProfileDefByCharacter(
	CharacterId characterId) noexcept
{
	for (const CharacterActionProfileDef& def : kCharacterActionProfiles)
	{
		if (def.characterId == characterId)
		{
			return &def;
		}
	}

	return nullptr;
}

std::span<const CharacterActionProfileDef> GetCharacterActionProfileDefs() noexcept
{
	return std::span<const CharacterActionProfileDef>(kCharacterActionProfiles);
}

const ActionInputBindingProfileDef* FindActionInputBindingProfileDef(
	ActionInputBindingProfileId id) noexcept
{
	for (const ActionInputBindingProfileDef& def : kActionInputBindingProfiles)
	{
		if (def.id == id)
		{
			return &def;
		}
	}

	return nullptr;
}

const ActionInputBindingProfileDef& GetActionInputBindingProfileDef(
	ActionInputBindingProfileId id)
{
	const ActionInputBindingProfileDef* const def =
		FindActionInputBindingProfileDef(id);
	if (def == nullptr)
	{
		throw std::out_of_range("ActionInputBindingProfileDef was not found.");
	}

	return *def;
}

std::span<const ActionInputBindingProfileDef> GetActionInputBindingProfileDefs() noexcept
{
	return std::span<const ActionInputBindingProfileDef>(kActionInputBindingProfiles);
}

const ActionFallbackReactionProfileDef* FindActionFallbackReactionProfileDef(
	ActionFallbackReactionProfileId id) noexcept
{
	for (const ActionFallbackReactionProfileDef& def :
		kActionFallbackReactionProfiles)
	{
		if (def.id == id)
		{
			return &def;
		}
	}

	return nullptr;
}

const ActionFallbackReactionProfileDef& GetActionFallbackReactionProfileDef(
	ActionFallbackReactionProfileId id)
{
	const ActionFallbackReactionProfileDef* const def =
		FindActionFallbackReactionProfileDef(id);
	if (def == nullptr)
	{
		throw std::out_of_range("ActionFallbackReactionProfileDef was not found.");
	}

	return *def;
}

std::span<const ActionFallbackReactionProfileDef> GetActionFallbackReactionProfileDefs() noexcept
{
	return std::span<const ActionFallbackReactionProfileDef>(
		kActionFallbackReactionProfiles);
}

const AnimationBindingProfileDef* FindAnimationBindingProfileDef(
	AnimationBindingProfileId id) noexcept
{
	for (const AnimationBindingProfileDef& def : kAnimationBindingProfiles)
	{
		if (def.id == id)
		{
			return &def;
		}
	}

	return nullptr;
}

const AnimationBindingProfileDef& GetAnimationBindingProfileDef(
	AnimationBindingProfileId id)
{
	const AnimationBindingProfileDef* const def =
		FindAnimationBindingProfileDef(id);
	if (def == nullptr)
	{
		throw std::out_of_range("AnimationBindingProfileDef was not found.");
	}

	return *def;
}

std::span<const AnimationBindingProfileDef> GetAnimationBindingProfileDefs() noexcept
{
	return std::span<const AnimationBindingProfileDef>(kAnimationBindingProfiles);
}
