#include "pch.h"
#include "AbilityDef.h"

#include "DefCompilePipeline.h"
#include "DefJsonFileLoader.h"
#include "DefJsonReader.h"
#include "DefKeyIndex.h"

#include <algorithm>

using json = nlohmann::json;

namespace
{
	struct GameplayTagQueryDto
	{
		GameplayTagQueryOp op{ GameplayTagQueryOp::None };
		std::vector<std::string> tagKeys;
		std::vector<GameplayTagQueryDto> children;
	};

	struct AbilityCostDto
	{
		std::string attributeKey;
		AbilityCostConsumeTiming consumeTiming{
			AbilityCostConsumeTiming::OnRequest };
		float amount{ 0.0f };
	};

	struct AbilityActivationDto
	{
		GameplayTagQueryDto requiredOwnerTags;
		GameplayTagQueryDto blockedOwnerTags;
		bool requiresTarget{ false };
		bool requiresGrounded{ false };
	};

	struct AbilityTransitionRuleDto
	{
		AbilityTransitionCause cause{ AbilityTransitionCause::None };
		std::string toAbilityKey;
		AbilityTransitionWindowPolicy windowPolicy{
			AbilityTransitionWindowPolicy::Always };
		std::optional<float> windowStartNormalized;
		std::optional<float> windowEndNormalized;
		int priority{ 0 };
		bool aiInterruptible{ false };
	};

	struct AbilityEndPolicyDto
	{
		AbilityEndType endType{ AbilityEndType::NaturalEnd };
		std::optional<std::string> defaultNextAbilityKey;
	};

	struct AbilityTransitionDto
	{
		AbilityEndPolicyDto endPolicy;
		std::optional<std::string> defaultNextAbilityKey;
		std::vector<AbilityTransitionRuleDto> interruptRules;
		std::vector<AbilityTransitionRuleDto> cancelRules;
	};

	struct AbilityCombatSpatialFilterDto
	{
		std::optional<float> facingHalfAngleDeg;
		std::optional<float> minDistance;
		std::optional<float> maxDistance;
		std::optional<float> verticalTolerance;
		AbilityCombatReferenceFrame referenceFrame{
			AbilityCombatReferenceFrame::OwnerFacing };
	};

	struct AbilityAttackHitDto
	{
		float damageScale{ 0.0f };
		float bonusDamage{ 0.0f };
		float staminaDamageScale{ 0.0f };
		float bonusStaminaDamage{ 0.0f };
		float poiseDamageScale{ 0.0f };
		float bonusPoiseDamage{ 0.0f };
		float knockbackDistance{ 0.0f };
		float hitStopSec{ 0.0f };
		bool parryable{ false };
		bool guardable{ false };
	};

	struct AbilityParryResponseDto
	{
		float stunSec{ 0.0f };
		float hitStopSec{ 0.0f };
		std::optional<std::string> grantEffectKey;
	};

	struct AbilityGuardResponseDto
	{
		float damageReductionRatio{ 0.0f };
		float chipDamageRatio{ 0.0f };
		float staminaDamageMultiplier{ 0.0f };
		float hitStopSec{ 0.0f };
	};

	struct AbilityCombatEffectDto
	{
		AbilityCombatEffectType type{ AbilityCombatEffectType::None };
		std::optional<AbilityAttackHitDto> attackHit;
		std::optional<AbilityParryResponseDto> parryResponse;
		std::optional<AbilityGuardResponseDto> guardResponse;
	};

	struct AbilityCombatWindowDto
	{
		AbilityCombatWindowKind kind{ AbilityCombatWindowKind::None };
		float startNormalized{ 0.0f };
		float endNormalized{ 0.0f };
		std::optional<std::string> applyEffectKey;
		std::optional<AbilityCombatApplyTo> appliesTo;
		std::optional<AbilityCombatSpatialFilterDto> spatialFilter;
		std::optional<AbilityCombatEffectDto> effect;
	};

	struct AbilityMovementSegmentDto
	{
		float startNormalized{ 0.0f };
		float endNormalized{ 0.0f };
		AbilityMovementMode movementMode{ AbilityMovementMode::None };
		std::optional<float> moveDistance;
		AbilityRotationMode rotationMode{ AbilityRotationMode::None };
		std::optional<float> rotationRate;
		AbilityVerticalMovementMode verticalMovementMode{
			AbilityVerticalMovementMode::None };
		std::optional<float> verticalAmount;
		AbilityDirectionPolicy directionPolicy{
			AbilityDirectionPolicy::ActionStartInput };
		AbilityDirectionSampleTiming directionSampleTiming{
			AbilityDirectionSampleTiming::OnSegmentStart };
	};

	struct AbilityEventDto
	{
		AbilityEventKind kind{ AbilityEventKind::None };
		float timeNormalized{ 0.0f };
		std::optional<std::string> effectKey;
		std::optional<std::string> cueTagKey;
		std::optional<uint16_t> payloadId;
		AbilityEventTriggerCondition condition{
			AbilityEventTriggerCondition::Always };
	};

	struct AbilityTimelineDto
	{
		float durationSec{ 0.0f };
		AbilityTimelinePolicy policy{ AbilityTimelinePolicy::FixedDuration };
		std::vector<AbilityCombatWindowDto> combatWindows;
		std::vector<AbilityMovementSegmentDto> movementSegments;
		std::vector<AbilityEventDto> events;
	};

	struct AbilityDefDto
	{
		std::string key;
		std::string name;
		AbilityKind kind{ AbilityKind::None };
		std::vector<std::string> tagKeys;
		AbilityActivationDto activation;
		std::vector<AbilityCostDto> costs;
		AbilityTimelineDto timeline;
		AbilityTransitionDto transition;
	};

	template<typename TEnum, typename TParser>
	bool ReadTypedString(
		const json& node,
		const char* field,
		TEnum& outValue,
		const char* typeName,
		TParser parser,
		std::string& outError)
	{
		std::string text;
		if (!ReadRequiredString(node, field, text, outError))
			return false;

		if (!parser(text, outValue))
		{
			outError = std::string("Unknown ") + typeName + ": " + text;
			return false;
		}

		return true;
	}

	template<typename TValue>
	bool ReadNullableNumber(
		const json& node,
		const char* field,
		std::optional<TValue>& outValue,
		std::string& outError)
	{
		if (!node.contains(field) || node.at(field).is_null())
		{
			outValue = std::nullopt;
			return true;
		}

		if (!node.at(field).is_number())
		{
			outError = std::string("Invalid numeric field: ") + field;
			return false;
		}

		outValue = node.at(field).get<TValue>();
		return true;
	}

	bool ReadNullableString(
		const json& node,
		const char* field,
		std::optional<std::string>& outValue,
		std::string& outError)
	{
		if (!node.contains(field) || node.at(field).is_null())
		{
			outValue = std::nullopt;
			return true;
		}

		if (!node.at(field).is_string())
		{
			outError = std::string("Invalid string field: ") + field;
			return false;
		}

		outValue = node.at(field).get<std::string>();
		return true;
	}

	template<typename TEnum, typename TParser>
	bool ReadNullableTypedString(
		const json& node,
		const char* field,
		std::optional<TEnum>& outValue,
		const char* typeName,
		TParser parser,
		std::string& outError)
	{
		if (!node.contains(field) || node.at(field).is_null())
		{
			outValue = std::nullopt;
			return true;
		}

		if (!node.at(field).is_string())
		{
			outError = std::string("Invalid string field: ") + field;
			return false;
		}

		TEnum parsedValue{};
		const std::string text = node.at(field).get<std::string>();
		if (!parser(text, parsedValue))
		{
			outError = std::string("Unknown ") + typeName + ": " + text;
			return false;
		}

		outValue = parsedValue;
		return true;
	}

	bool ParseAbilityKind(
		std::string_view text,
		AbilityKind& outValue) noexcept
	{
		if (text == "Attack") { outValue = AbilityKind::Attack; return true; }
		if (text == "Dodge") { outValue = AbilityKind::Dodge; return true; }
		if (text == "Parry") { outValue = AbilityKind::Parry; return true; }
		if (text == "Guard") { outValue = AbilityKind::Guard; return true; }
		if (text == "Reaction") { outValue = AbilityKind::Reaction; return true; }
		if (text == "UseItem") { outValue = AbilityKind::UseItem; return true; }
		if (text == "NonCombat") { outValue = AbilityKind::NonCombat; return true; }
		if (text == "Dead") { outValue = AbilityKind::Dead; return true; }
		if (text == "None") { outValue = AbilityKind::None; return true; }
		return false;
	}

	bool ParseTimelinePolicy(
		std::string_view text,
		AbilityTimelinePolicy& outValue) noexcept
	{
		if (text == "FixedDuration") { outValue = AbilityTimelinePolicy::FixedDuration; return true; }
		if (text == "Holdable") { outValue = AbilityTimelinePolicy::Holdable; return true; }
		return false;
	}

	bool ParseCostTiming(
		std::string_view text,
		AbilityCostConsumeTiming& outValue) noexcept
	{
		if (text == "OnRequest") { outValue = AbilityCostConsumeTiming::OnRequest; return true; }
		if (text == "OnCommit") { outValue = AbilityCostConsumeTiming::OnCommit; return true; }
		if (text == "OnWindowEnter") { outValue = AbilityCostConsumeTiming::OnWindowEnter; return true; }
		return false;
	}

	bool ParseTransitionCause(
		std::string_view text,
		AbilityTransitionCause& outValue) noexcept
	{
		if (text == "OnHitReceived") { outValue = AbilityTransitionCause::OnHitReceived; return true; }
		if (text == "OnParried") { outValue = AbilityTransitionCause::OnParried; return true; }
		if (text == "OnAttributeZero") { outValue = AbilityTransitionCause::OnAttributeZero; return true; }
		if (text == "HoldRelease") { outValue = AbilityTransitionCause::HoldRelease; return true; }
		if (text == "ManualCancel") { outValue = AbilityTransitionCause::ManualCancel; return true; }
		if (text == "Combo") { outValue = AbilityTransitionCause::Combo; return true; }
		if (text == "LightAttackCancel") { outValue = AbilityTransitionCause::LightAttackCancel; return true; }
		if (text == "HeavyAttackCancel") { outValue = AbilityTransitionCause::HeavyAttackCancel; return true; }
		if (text == "DodgeCancel") { outValue = AbilityTransitionCause::DodgeCancel; return true; }
		if (text == "ParryCancel") { outValue = AbilityTransitionCause::ParryCancel; return true; }
		if (text == "None") { outValue = AbilityTransitionCause::None; return true; }
		return false;
	}

	bool ParseTransitionWindowPolicy(
		std::string_view text,
		AbilityTransitionWindowPolicy& outValue) noexcept
	{
		if (text == "Always") { outValue = AbilityTransitionWindowPolicy::Always; return true; }
		if (text == "Range") { outValue = AbilityTransitionWindowPolicy::Range; return true; }
		return false;
	}

	bool ParseEndType(
		std::string_view text,
		AbilityEndType& outValue) noexcept
	{
		if (text == "NaturalEnd") { outValue = AbilityEndType::NaturalEnd; return true; }
		if (text == "HoldRelease") { outValue = AbilityEndType::HoldRelease; return true; }
		if (text == "ImmediateTransition") { outValue = AbilityEndType::ImmediateTransition; return true; }
		return false;
	}

	bool ParseMovementMode(
		std::string_view text,
		AbilityMovementMode& outValue) noexcept
	{
		if (text == "None") { outValue = AbilityMovementMode::None; return true; }
		if (text == "ForwardFixedDistance") { outValue = AbilityMovementMode::ForwardFixedDistance; return true; }
		if (text == "InputDirectionDistance") { outValue = AbilityMovementMode::InputDirectionDistance; return true; }
		return false;
	}

	bool ParseRotationMode(
		std::string_view text,
		AbilityRotationMode& outValue) noexcept
	{
		if (text == "None") { outValue = AbilityRotationMode::None; return true; }
		if (text == "FaceMoveDirection") { outValue = AbilityRotationMode::FaceMoveDirection; return true; }
		if (text == "FaceTarget") { outValue = AbilityRotationMode::FaceTarget; return true; }
		return false;
	}

	bool ParseVerticalMovementMode(
		std::string_view text,
		AbilityVerticalMovementMode& outValue) noexcept
	{
		if (text == "None") { outValue = AbilityVerticalMovementMode::None; return true; }
		if (text == "FixedOffset") { outValue = AbilityVerticalMovementMode::FixedOffset; return true; }
		return false;
	}

	bool ParseDirectionPolicy(
		std::string_view text,
		AbilityDirectionPolicy& outValue) noexcept
	{
		if (text == "ActionStartInput") { outValue = AbilityDirectionPolicy::ActionStartInput; return true; }
		if (text == "CurrentInput") { outValue = AbilityDirectionPolicy::CurrentInput; return true; }
		if (text == "FacingDirection") { outValue = AbilityDirectionPolicy::FacingDirection; return true; }
		if (text == "TargetDirection") { outValue = AbilityDirectionPolicy::TargetDirection; return true; }
		if (text == "LockedDirection") { outValue = AbilityDirectionPolicy::LockedDirection; return true; }
		return false;
	}

	bool ParseDirectionSampleTiming(
		std::string_view text,
		AbilityDirectionSampleTiming& outValue) noexcept
	{
		if (text == "OnSegmentStart") { outValue = AbilityDirectionSampleTiming::OnSegmentStart; return true; }
		if (text == "Continuous") { outValue = AbilityDirectionSampleTiming::Continuous; return true; }
		return false;
	}

	bool ParseCombatWindowKind(
		std::string_view text,
		AbilityCombatWindowKind& outValue) noexcept
	{
		if (text == "Attack") { outValue = AbilityCombatWindowKind::Attack; return true; }
		if (text == "Parry") { outValue = AbilityCombatWindowKind::Parry; return true; }
		if (text == "Guard") { outValue = AbilityCombatWindowKind::Guard; return true; }
		if (text == "Armor") { outValue = AbilityCombatWindowKind::Armor; return true; }
		if (text == "Invulnerability") { outValue = AbilityCombatWindowKind::Invulnerability; return true; }
		if (text == "None") { outValue = AbilityCombatWindowKind::None; return true; }
		return false;
	}

	bool ParseCombatApplyTo(
		std::string_view text,
		AbilityCombatApplyTo& outValue) noexcept
	{
		if (text == "FrontPhysical") { outValue = AbilityCombatApplyTo::FrontPhysical; return true; }
		if (text == "ParryableAttack") { outValue = AbilityCombatApplyTo::ParryableAttack; return true; }
		if (text == "GuardableAttack") { outValue = AbilityCombatApplyTo::GuardableAttack; return true; }
		return false;
	}

	bool ParseCombatReferenceFrame(
		std::string_view text,
		AbilityCombatReferenceFrame& outValue) noexcept
	{
		if (text == "OwnerFacing") { outValue = AbilityCombatReferenceFrame::OwnerFacing; return true; }
		if (text == "MoveDirection") { outValue = AbilityCombatReferenceFrame::MoveDirection; return true; }
		if (text == "LockedActionDirection") { outValue = AbilityCombatReferenceFrame::LockedActionDirection; return true; }
		return false;
	}

	bool ParseCombatEffectType(
		std::string_view text,
		AbilityCombatEffectType& outValue) noexcept
	{
		if (text == "None") { outValue = AbilityCombatEffectType::None; return true; }
		if (text == "AttackHit") { outValue = AbilityCombatEffectType::AttackHit; return true; }
		if (text == "ParryResponse") { outValue = AbilityCombatEffectType::ParryResponse; return true; }
		if (text == "GuardResponse") { outValue = AbilityCombatEffectType::GuardResponse; return true; }
		return false;
	}

	bool ParseEventKind(
		std::string_view text,
		AbilityEventKind& outValue) noexcept
	{
		if (text == "ConsumeItem") { outValue = AbilityEventKind::ConsumeItem; return true; }
		if (text == "ApplyGameplayEffect") { outValue = AbilityEventKind::ApplyGameplayEffect; return true; }
		if (text == "SpawnProjectile") { outValue = AbilityEventKind::SpawnProjectile; return true; }
		if (text == "PlayCue") { outValue = AbilityEventKind::PlayCue; return true; }
		if (text == "None") { outValue = AbilityEventKind::None; return true; }
		return false;
	}

	bool ParseEventTriggerCondition(
		std::string_view text,
		AbilityEventTriggerCondition& outValue) noexcept
	{
		if (text == "Always") { outValue = AbilityEventTriggerCondition::Always; return true; }
		if (text == "OnParrySuccess") { outValue = AbilityEventTriggerCondition::OnParrySuccess; return true; }
		return false;
	}

	bool ParseStringArray(
		const json& node,
		std::vector<std::string>& outValues,
		std::string& outError)
	{
		if (!node.is_array())
		{
			outError = "Expected string array.";
			return false;
		}

		outValues.clear();
		outValues.reserve(node.size());
		for (const json& item : node)
		{
			if (!item.is_string())
			{
				outError = "String array entry must be a string.";
				return false;
			}

			outValues.push_back(item.get<std::string>());
		}

		return true;
	}

	bool ParseTagQuery(
		const json& node,
		GameplayTagQueryDto& outQuery,
		std::string& outError)
	{
		if (node.is_null())
		{
			outQuery = {};
			return true;
		}

		if (!node.is_object())
		{
			outError = "Gameplay tag query must be an object or null.";
			return false;
		}

		std::string opText;
		if (!ReadRequiredString(node, "op", opText, outError))
			return false;

		if (opText == "None") { outQuery.op = GameplayTagQueryOp::None; }
		else if (opText == "All") { outQuery.op = GameplayTagQueryOp::All; }
		else if (opText == "Any") { outQuery.op = GameplayTagQueryOp::Any; }
		else if (opText == "Not") { outQuery.op = GameplayTagQueryOp::Not; }
		else
		{
			outError = "Unknown GameplayTagQueryOp: " + opText;
			return false;
		}

		if (node.contains("tags") &&
			!ParseStringArray(node.at("tags"), outQuery.tagKeys, outError))
		{
			return false;
		}

		if (node.contains("children"))
		{
			if (!node.at("children").is_array())
			{
				outError = "Gameplay tag query children must be an array.";
				return false;
			}

			outQuery.children.clear();
			for (const json& childNode : node.at("children"))
			{
				GameplayTagQueryDto child{};
				if (!ParseTagQuery(childNode, child, outError))
					return false;

				outQuery.children.push_back(std::move(child));
			}
		}

		return true;
	}

	template<typename TDto, typename TParser>
	bool ParseArray(
		const json& node,
		const char* field,
		std::vector<TDto>& outDtos,
		TParser parser,
		std::string& outError)
	{
		outDtos.clear();
		if (!node.contains(field))
			return true;

		if (!node.at(field).is_array())
		{
			outError = std::string("Invalid array field: ") + field;
			return false;
		}

		for (const json& item : node.at(field))
		{
			TDto dto{};
			if (!parser(item, dto, outError))
				return false;

			outDtos.push_back(std::move(dto));
		}

		return true;
	}

	bool ParseCost(
		const json& node,
		AbilityCostDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredString(node, "attributeKey", outDto.attributeKey, outError) &&
			ReadTypedString(
				node,
				"consumeTiming",
				outDto.consumeTiming,
				"AbilityCostConsumeTiming",
				ParseCostTiming,
				outError) &&
			ReadRequiredNumber(node, "amount", outDto.amount, outError);
	}

	bool ParseActivation(
		const json& node,
		AbilityActivationDto& outDto,
		std::string& outError)
	{
		if (!node.is_object())
		{
			outError = "Ability activation must be an object.";
			return false;
		}

		if (node.contains("requiredOwnerTags") &&
			!ParseTagQuery(
				node.at("requiredOwnerTags"),
				outDto.requiredOwnerTags,
				outError))
		{
			return false;
		}

		if (node.contains("blockedOwnerTags") &&
			!ParseTagQuery(
				node.at("blockedOwnerTags"),
				outDto.blockedOwnerTags,
				outError))
		{
			return false;
		}

		return
			ReadRequiredBool(
				node,
				"requiresTarget",
				outDto.requiresTarget,
				outError) &&
			ReadRequiredBool(
				node,
				"requiresGrounded",
				outDto.requiresGrounded,
				outError);
	}

	bool ParseTransitionRule(
		const json& node,
		AbilityTransitionRuleDto& outDto,
		std::string& outError)
	{
		return
			ReadTypedString(
				node,
				"cause",
				outDto.cause,
				"AbilityTransitionCause",
				ParseTransitionCause,
				outError) &&
			ReadRequiredString(node, "toAbilityKey", outDto.toAbilityKey, outError) &&
			ReadTypedString(
				node,
				"windowPolicy",
				outDto.windowPolicy,
				"AbilityTransitionWindowPolicy",
				ParseTransitionWindowPolicy,
				outError) &&
			ReadNullableNumber(
				node,
				"windowStartNormalized",
				outDto.windowStartNormalized,
				outError) &&
			ReadNullableNumber(
				node,
				"windowEndNormalized",
				outDto.windowEndNormalized,
				outError) &&
			ReadRequiredNumber(node, "priority", outDto.priority, outError) &&
			ReadRequiredBool(
				node,
				"aiInterruptible",
				outDto.aiInterruptible,
				outError);
	}

	bool ParseEndPolicy(
		const json& node,
		AbilityEndPolicyDto& outDto,
		std::string& outError)
	{
		if (!node.is_object())
		{
			outError = "Ability end policy must be an object.";
			return false;
		}

		return
			ReadTypedString(
				node,
				"endType",
				outDto.endType,
				"AbilityEndType",
				ParseEndType,
				outError) &&
			ReadNullableString(
				node,
				"defaultNextAbilityKey",
				outDto.defaultNextAbilityKey,
				outError);
	}

	bool ParseTransition(
		const json& node,
		AbilityTransitionDto& outDto,
		std::string& outError)
	{
		if (!node.is_object())
		{
			outError = "Ability transition must be an object.";
			return false;
		}

		if (node.contains("endPolicy") &&
			!ParseEndPolicy(node.at("endPolicy"), outDto.endPolicy, outError))
		{
			return false;
		}

		return
			ReadNullableString(
				node,
				"defaultNextAbilityKey",
				outDto.defaultNextAbilityKey,
				outError) &&
			ParseArray<AbilityTransitionRuleDto>(
				node,
				"interruptRules",
				outDto.interruptRules,
				ParseTransitionRule,
				outError) &&
			ParseArray<AbilityTransitionRuleDto>(
				node,
				"cancelRules",
				outDto.cancelRules,
				ParseTransitionRule,
				outError);
	}

	bool ParseCombatSpatialFilter(
		const json& node,
		AbilityCombatSpatialFilterDto& outDto,
		std::string& outError)
	{
		if (!node.is_object())
		{
			outError = "Ability combat spatial filter must be an object.";
			return false;
		}

		return
			ReadNullableNumber(
				node,
				"facingHalfAngleDeg",
				outDto.facingHalfAngleDeg,
				outError) &&
			ReadNullableNumber(
				node,
				"minDistance",
				outDto.minDistance,
				outError) &&
			ReadNullableNumber(
				node,
				"maxDistance",
				outDto.maxDistance,
				outError) &&
			ReadNullableNumber(
				node,
				"verticalTolerance",
				outDto.verticalTolerance,
				outError) &&
			ReadTypedString(
				node,
				"referenceFrame",
				outDto.referenceFrame,
				"AbilityCombatReferenceFrame",
				ParseCombatReferenceFrame,
				outError);
	}

	bool ParseAttackHit(
		const json& node,
		AbilityAttackHitDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredNumber(
				node,
				"damageScale",
				outDto.damageScale,
				outError) &&
			ReadRequiredNumber(
				node,
				"bonusDamage",
				outDto.bonusDamage,
				outError) &&
			ReadRequiredNumber(
				node,
				"staminaDamageScale",
				outDto.staminaDamageScale,
				outError) &&
			ReadRequiredNumber(
				node,
				"bonusStaminaDamage",
				outDto.bonusStaminaDamage,
				outError) &&
			ReadRequiredNumber(
				node,
				"poiseDamageScale",
				outDto.poiseDamageScale,
				outError) &&
			ReadRequiredNumber(
				node,
				"bonusPoiseDamage",
				outDto.bonusPoiseDamage,
				outError) &&
			ReadRequiredNumber(
				node,
				"knockbackDistance",
				outDto.knockbackDistance,
				outError) &&
			ReadRequiredNumber(
				node,
				"hitStopSec",
				outDto.hitStopSec,
				outError) &&
			ReadRequiredBool(node, "parryable", outDto.parryable, outError) &&
			ReadRequiredBool(node, "guardable", outDto.guardable, outError);
	}

	bool ParseParryResponse(
		const json& node,
		AbilityParryResponseDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredNumber(node, "stunSec", outDto.stunSec, outError) &&
			ReadRequiredNumber(node, "hitStopSec", outDto.hitStopSec, outError) &&
			ReadNullableString(
				node,
				"grantEffectKey",
				outDto.grantEffectKey,
				outError);
	}

	bool ParseGuardResponse(
		const json& node,
		AbilityGuardResponseDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredNumber(
				node,
				"damageReductionRatio",
				outDto.damageReductionRatio,
				outError) &&
			ReadRequiredNumber(
				node,
				"chipDamageRatio",
				outDto.chipDamageRatio,
				outError) &&
			ReadRequiredNumber(
				node,
				"staminaDamageMultiplier",
				outDto.staminaDamageMultiplier,
				outError) &&
			ReadRequiredNumber(node, "hitStopSec", outDto.hitStopSec, outError);
	}

	bool ParseCombatEffect(
		const json& node,
		AbilityCombatEffectDto& outDto,
		std::string& outError)
	{
		if (!node.is_object())
		{
			outError = "Ability combat effect must be an object.";
			return false;
		}

		if (!ReadTypedString(
				node,
				"type",
				outDto.type,
				"AbilityCombatEffectType",
				ParseCombatEffectType,
				outError))
		{
			return false;
		}

		if (node.contains("attackHit") && !node.at("attackHit").is_null())
		{
			AbilityAttackHitDto attackHit{};
			if (!ParseAttackHit(node.at("attackHit"), attackHit, outError))
				return false;

			outDto.attackHit = std::move(attackHit);
		}

		if (node.contains("parryResponse") &&
			!node.at("parryResponse").is_null())
		{
			AbilityParryResponseDto parryResponse{};
			if (!ParseParryResponse(
					node.at("parryResponse"),
					parryResponse,
					outError))
			{
				return false;
			}

			outDto.parryResponse = std::move(parryResponse);
		}

		if (node.contains("guardResponse") &&
			!node.at("guardResponse").is_null())
		{
			AbilityGuardResponseDto guardResponse{};
			if (!ParseGuardResponse(
					node.at("guardResponse"),
					guardResponse,
					outError))
			{
				return false;
			}

			outDto.guardResponse = std::move(guardResponse);
		}

		return true;
	}

	bool ParseCombatWindow(
		const json& node,
		AbilityCombatWindowDto& outDto,
		std::string& outError)
	{
		if (!ReadTypedString(
				node,
				"kind",
				outDto.kind,
				"AbilityCombatWindowKind",
				ParseCombatWindowKind,
				outError) ||
			!ReadRequiredNumber(
				node,
				"startNormalized",
				outDto.startNormalized,
				outError) ||
			!ReadRequiredNumber(
				node,
				"endNormalized",
				outDto.endNormalized,
				outError) ||
			!ReadNullableString(
				node,
				"applyEffectKey",
				outDto.applyEffectKey,
				outError) ||
			!ReadNullableTypedString(
				node,
				"appliesTo",
				outDto.appliesTo,
				"AbilityCombatApplyTo",
				ParseCombatApplyTo,
				outError))
		{
			return false;
		}

		if (node.contains("spatialFilter") &&
			!node.at("spatialFilter").is_null())
		{
			AbilityCombatSpatialFilterDto spatialFilter{};
			if (!ParseCombatSpatialFilter(
					node.at("spatialFilter"),
					spatialFilter,
					outError))
			{
				return false;
			}

			outDto.spatialFilter = std::move(spatialFilter);
		}

		if (node.contains("effect") && !node.at("effect").is_null())
		{
			AbilityCombatEffectDto effect{};
			if (!ParseCombatEffect(node.at("effect"), effect, outError))
				return false;

			outDto.effect = std::move(effect);
		}

		return true;
	}

	bool ParseMovementSegment(
		const json& node,
		AbilityMovementSegmentDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredNumber(
				node,
				"startNormalized",
				outDto.startNormalized,
				outError) &&
			ReadRequiredNumber(
				node,
				"endNormalized",
				outDto.endNormalized,
				outError) &&
			ReadTypedString(
				node,
				"movementMode",
				outDto.movementMode,
				"AbilityMovementMode",
				ParseMovementMode,
				outError) &&
			ReadNullableNumber(
				node,
				"moveDistance",
				outDto.moveDistance,
				outError) &&
			ReadTypedString(
				node,
				"rotationMode",
				outDto.rotationMode,
				"AbilityRotationMode",
				ParseRotationMode,
				outError) &&
			ReadNullableNumber(
				node,
				"rotationRate",
				outDto.rotationRate,
				outError) &&
			ReadTypedString(
				node,
				"verticalMovementMode",
				outDto.verticalMovementMode,
				"AbilityVerticalMovementMode",
				ParseVerticalMovementMode,
				outError) &&
			ReadNullableNumber(
				node,
				"verticalAmount",
				outDto.verticalAmount,
				outError) &&
			ReadTypedString(
				node,
				"directionPolicy",
				outDto.directionPolicy,
				"AbilityDirectionPolicy",
				ParseDirectionPolicy,
				outError) &&
			ReadTypedString(
				node,
				"directionSampleTiming",
				outDto.directionSampleTiming,
				"AbilityDirectionSampleTiming",
				ParseDirectionSampleTiming,
				outError);
	}

	bool ParseAbilityEvent(
		const json& node,
		AbilityEventDto& outDto,
		std::string& outError)
	{
		return
			ReadTypedString(
				node,
				"kind",
				outDto.kind,
				"AbilityEventKind",
				ParseEventKind,
				outError) &&
			ReadRequiredNumber(
				node,
				"timeNormalized",
				outDto.timeNormalized,
				outError) &&
			ReadNullableString(node, "effectKey", outDto.effectKey, outError) &&
			ReadNullableString(node, "cueTagKey", outDto.cueTagKey, outError) &&
			ReadNullableNumber(node, "payloadId", outDto.payloadId, outError) &&
			ReadTypedString(
				node,
				"condition",
				outDto.condition,
				"AbilityEventTriggerCondition",
				ParseEventTriggerCondition,
				outError);
	}

	bool ParseTimeline(
		const json& node,
		AbilityTimelineDto& outDto,
		std::string& outError)
	{
		if (!node.is_object())
		{
			outError = "Ability timeline must be an object.";
			return false;
		}

		return
			ReadRequiredNumber(node, "durationSec", outDto.durationSec, outError) &&
			ReadTypedString(
				node,
				"policy",
				outDto.policy,
				"AbilityTimelinePolicy",
				ParseTimelinePolicy,
				outError) &&
			ParseArray<AbilityCombatWindowDto>(
				node,
				"combatWindows",
				outDto.combatWindows,
				ParseCombatWindow,
				outError) &&
			ParseArray<AbilityMovementSegmentDto>(
				node,
				"movementSegments",
				outDto.movementSegments,
				ParseMovementSegment,
				outError) &&
			ParseArray<AbilityEventDto>(
				node,
				"events",
				outDto.events,
				ParseAbilityEvent,
				outError);
	}

	bool ParseAbility(
		const json& node,
		AbilityDefDto& outDto,
		std::string& outError)
	{
		if (!ReadRequiredString(node, "key", outDto.key, outError) ||
			!ReadRequiredString(node, "name", outDto.name, outError) ||
			!ReadTypedString(
				node,
				"kind",
				outDto.kind,
				"AbilityKind",
				ParseAbilityKind,
				outError))
		{
			return false;
		}

		if (node.contains("tags") &&
			!ParseStringArray(node.at("tags"), outDto.tagKeys, outError))
		{
			return false;
		}

		return
			node.contains("activation") &&
			ParseActivation(node.at("activation"), outDto.activation, outError) &&
			ParseArray<AbilityCostDto>(
				node,
				"costs",
				outDto.costs,
				ParseCost,
				outError) &&
			node.contains("timeline") &&
			ParseTimeline(node.at("timeline"), outDto.timeline, outError) &&
			node.contains("transition") &&
			ParseTransition(node.at("transition"), outDto.transition, outError);
	}

	bool AppendAbilityDocumentDtos(
		const json& root,
		std::vector<AbilityDefDto>& outDtos,
		std::string& outError)
	{
		if (!root.contains("abilities") || !root.at("abilities").is_array())
		{
			outError = "Missing or invalid array field: abilities";
			return false;
		}

		for (const json& abilityNode : root.at("abilities"))
		{
			AbilityDefDto dto{};
			if (!ParseAbility(abilityNode, dto, outError))
				return false;

			outDtos.push_back(std::move(dto));
		}

		return true;
	}

	template<typename TDef, typename TId>
	bool ResolveDefIdByKey(
		std::span<const TDef> defs,
		std::string_view key,
		TId& outId,
		const char* typeName,
		std::string& outError)
	{
		for (const TDef& def : defs)
		{
			if (def.key == key)
			{
				outId = def.id;
				return true;
			}
		}

		outError = std::string("Unknown ") + typeName + " key: " +
			std::string(key);
		return false;
	}

	bool AddTagToMask(
		GameplayTagId tagId,
		GameplayTagMask& outMask,
		std::string& outError)
	{
		if (tagId == InvalidGameplayTagId || tagId > 64)
		{
			outError = "Gameplay tag id cannot be represented in GameplayTagMask.";
			return false;
		}

		outMask |= (GameplayTagMask{ 1 } << (tagId - 1));
		return true;
	}

	bool CompileTagQuery(
		const GameplayTagQueryDto& dto,
		std::span<const GameplayTagDef> tags,
		GameplayTagQueryDef& outQuery,
		std::string& outError)
	{
		outQuery.op = dto.op;
		outQuery.tags.clear();
		outQuery.tags.reserve(dto.tagKeys.size());

		for (const std::string& tagKey : dto.tagKeys)
		{
			GameplayTagId tagId{ InvalidGameplayTagId };
			if (!ResolveDefIdByKey(
					tags,
					tagKey,
					tagId,
					"gameplay tag",
					outError))
			{
				return false;
			}

			outQuery.tags.push_back(tagId);
		}

		outQuery.children.clear();
		outQuery.children.reserve(dto.children.size());
		for (const GameplayTagQueryDto& childDto : dto.children)
		{
			GameplayTagQueryDef child{};
			if (!CompileTagQuery(childDto, tags, child, outError))
				return false;

			outQuery.children.push_back(std::move(child));
		}

		return true;
	}

	bool CompileTagMask(
		std::span<const std::string> tagKeys,
		std::span<const GameplayTagDef> tags,
		GameplayTagMask& outMask,
		std::string& outError)
	{
		outMask = 0;
		for (const std::string& tagKey : tagKeys)
		{
			GameplayTagId tagId{ InvalidGameplayTagId };
			if (!ResolveDefIdByKey(
					tags,
					tagKey,
					tagId,
					"gameplay tag",
					outError) ||
				!AddTagToMask(tagId, outMask, outError))
			{
				return false;
			}
		}

		return true;
	}

	bool CompileCost(
		const AbilityCostDto& dto,
		std::span<const AttributeDef> attributes,
		AbilityCostDef& outDef,
		std::string& outError)
	{
		if (!ResolveDefIdByKey(
				attributes,
				dto.attributeKey,
				outDef.attributeId,
				"attribute",
				outError))
		{
			return false;
		}

		outDef.consumeTiming = dto.consumeTiming;
		outDef.amount = dto.amount;
		return true;
	}

	bool CompileTransitionRule(
		const AbilityTransitionRuleDto& dto,
		const DefKeyIndex<AbilityId>& abilityKeyIndex,
		AbilityTransitionRuleDef& outDef,
		std::string& outError)
	{
		if (!abilityKeyIndex.FindId(dto.toAbilityKey, outDef.toAbilityId))
		{
			outError = "Unknown ability transition target key: " +
				dto.toAbilityKey;
			return false;
		}

		outDef.cause = dto.cause;
		outDef.windowPolicy = dto.windowPolicy;
		outDef.windowStartNormalized = dto.windowStartNormalized;
		outDef.windowEndNormalized = dto.windowEndNormalized;
		outDef.priority = dto.priority;
		outDef.aiInterruptible = dto.aiInterruptible;
		return true;
	}

	bool CompileEndPolicy(
		const AbilityEndPolicyDto& dto,
		const DefKeyIndex<AbilityId>& abilityKeyIndex,
		AbilityEndPolicyDef& outDef,
		std::string& outError)
	{
		outDef.endType = dto.endType;
		outDef.defaultNextAbilityId = InvalidAbilityId;
		if (dto.defaultNextAbilityKey.has_value() &&
			!abilityKeyIndex.FindId(
				*dto.defaultNextAbilityKey,
				outDef.defaultNextAbilityId))
		{
			outError = "Unknown end policy default ability key: " +
				*dto.defaultNextAbilityKey;
			return false;
		}

		return true;
	}

	bool CompileParryResponse(
		const AbilityParryResponseDto& dto,
		std::span<const GameplayEffectDef> effects,
		AbilityParryResponseDef& outDef,
		std::string& outError)
	{
		outDef.stunSec = dto.stunSec;
		outDef.hitStopSec = dto.hitStopSec;
		outDef.grantEffectId = std::nullopt;
		if (dto.grantEffectKey.has_value())
		{
			GameplayEffectId effectId{ InvalidGameplayEffectId };
			if (!ResolveDefIdByKey(
					effects,
					*dto.grantEffectKey,
					effectId,
					"gameplay effect",
					outError))
			{
				return false;
			}

			outDef.grantEffectId = effectId;
		}

		return true;
	}

	bool CompileCombatEffect(
		const AbilityCombatEffectDto& dto,
		std::span<const GameplayEffectDef> effects,
		AbilityCombatEffectDef& outDef,
		std::string& outError)
	{
		outDef.type = dto.type;
		outDef.attackHit = std::nullopt;
		outDef.parryResponse = std::nullopt;
		outDef.guardResponse = std::nullopt;

		if (dto.attackHit.has_value())
		{
			AbilityAttackHitDef attackHit{};
			attackHit.damageScale = dto.attackHit->damageScale;
			attackHit.bonusDamage = dto.attackHit->bonusDamage;
			attackHit.staminaDamageScale = dto.attackHit->staminaDamageScale;
			attackHit.bonusStaminaDamage = dto.attackHit->bonusStaminaDamage;
			attackHit.poiseDamageScale = dto.attackHit->poiseDamageScale;
			attackHit.bonusPoiseDamage = dto.attackHit->bonusPoiseDamage;
			attackHit.knockbackDistance = dto.attackHit->knockbackDistance;
			attackHit.hitStopSec = dto.attackHit->hitStopSec;
			attackHit.parryable = dto.attackHit->parryable;
			attackHit.guardable = dto.attackHit->guardable;
			outDef.attackHit = attackHit;
		}

		if (dto.parryResponse.has_value())
		{
			AbilityParryResponseDef parryResponse{};
			if (!CompileParryResponse(
					*dto.parryResponse,
					effects,
					parryResponse,
					outError))
			{
				return false;
			}

			outDef.parryResponse = parryResponse;
		}

		if (dto.guardResponse.has_value())
		{
			AbilityGuardResponseDef guardResponse{};
			guardResponse.damageReductionRatio =
				dto.guardResponse->damageReductionRatio;
			guardResponse.chipDamageRatio = dto.guardResponse->chipDamageRatio;
			guardResponse.staminaDamageMultiplier =
				dto.guardResponse->staminaDamageMultiplier;
			guardResponse.hitStopSec = dto.guardResponse->hitStopSec;
			outDef.guardResponse = guardResponse;
		}

		return true;
	}

	bool CompileCombatWindow(
		const AbilityCombatWindowDto& dto,
		std::span<const GameplayEffectDef> effects,
		AbilityCombatWindowDef& outDef,
		std::string& outError)
	{
		outDef.kind = dto.kind;
		outDef.startNormalized = dto.startNormalized;
		outDef.endNormalized = dto.endNormalized;
		outDef.applyEffectId = std::nullopt;
		outDef.appliesTo = dto.appliesTo;
		outDef.spatialFilter = std::nullopt;
		outDef.effect = std::nullopt;

		if (dto.applyEffectKey.has_value())
		{
			GameplayEffectId effectId{ InvalidGameplayEffectId };
			if (!ResolveDefIdByKey(
					effects,
					*dto.applyEffectKey,
					effectId,
					"gameplay effect",
					outError))
			{
				return false;
			}

			outDef.applyEffectId = effectId;
		}

		if (dto.spatialFilter.has_value())
		{
			AbilityCombatSpatialFilterDef spatialFilter{};
			spatialFilter.facingHalfAngleDeg =
				dto.spatialFilter->facingHalfAngleDeg;
			spatialFilter.minDistance = dto.spatialFilter->minDistance;
			spatialFilter.maxDistance = dto.spatialFilter->maxDistance;
			spatialFilter.verticalTolerance =
				dto.spatialFilter->verticalTolerance;
			spatialFilter.referenceFrame = dto.spatialFilter->referenceFrame;
			outDef.spatialFilter = spatialFilter;
		}

		if (dto.effect.has_value())
		{
			AbilityCombatEffectDef effect{};
			if (!CompileCombatEffect(*dto.effect, effects, effect, outError))
				return false;

			outDef.effect = std::move(effect);
		}

		return true;
	}

	bool CompileMovementSegment(
		const AbilityMovementSegmentDto& dto,
		AbilityMovementSegmentDef& outDef,
		std::string& outError)
	{
		(void)outError;
		outDef.startNormalized = dto.startNormalized;
		outDef.endNormalized = dto.endNormalized;
		outDef.movementMode = dto.movementMode;
		outDef.moveDistance = dto.moveDistance;
		outDef.rotationMode = dto.rotationMode;
		outDef.rotationRate = dto.rotationRate;
		outDef.verticalMovementMode = dto.verticalMovementMode;
		outDef.verticalAmount = dto.verticalAmount;
		outDef.directionPolicy = dto.directionPolicy;
		outDef.directionSampleTiming = dto.directionSampleTiming;
		return true;
	}

	bool CompileEvent(
		const AbilityEventDto& dto,
		std::span<const GameplayEffectDef> effects,
		std::span<const GameplayTagDef> tags,
		AbilityEventDef& outDef,
		std::string& outError)
	{
		outDef.kind = dto.kind;
		outDef.timeNormalized = dto.timeNormalized;
		outDef.effectId = std::nullopt;
		outDef.cueTagId = std::nullopt;
		outDef.payloadId = dto.payloadId;
		outDef.condition = dto.condition;

		if (dto.effectKey.has_value())
		{
			GameplayEffectId effectId{ InvalidGameplayEffectId };
			if (!ResolveDefIdByKey(
					effects,
					*dto.effectKey,
					effectId,
					"gameplay effect",
					outError))
			{
				return false;
			}

			outDef.effectId = effectId;
		}

		if (dto.cueTagKey.has_value())
		{
			GameplayTagId tagId{ InvalidGameplayTagId };
			if (!ResolveDefIdByKey(
					tags,
					*dto.cueTagKey,
					tagId,
					"gameplay tag",
					outError))
			{
				return false;
			}

			outDef.cueTagId = tagId;
		}

		return true;
	}

	template<typename TDto, typename TDef, typename TCompiler>
	bool CompileVector(
		std::span<const TDto> dtos,
		std::vector<TDef>& outDefs,
		TCompiler compiler,
		std::string& outError)
	{
		outDefs.clear();
		outDefs.reserve(dtos.size());
		for (const TDto& dto : dtos)
		{
			TDef def{};
			if (!compiler(dto, def, outError))
				return false;

			outDefs.push_back(std::move(def));
		}

		return true;
	}

	bool BuildAbilityKeyIndex(
		std::span<const DefParsedDto<AbilityDefDto>> dtos,
		DefKeyIndex<AbilityId>& outIndex,
		std::string& outError)
	{
		std::vector<std::string> keys;
		keys.reserve(dtos.size());
		for (const DefParsedDto<AbilityDefDto>& parsedDto : dtos)
			keys.push_back(parsedDto.dto.key);

		return BuildDeterministicDefKeyIndex(
			std::span<const std::string>(keys.data(), keys.size()),
			static_cast<AbilityId>(InvalidAbilityId + 1),
			outIndex,
			outError);
	}

	bool CompileAbilityDef(
		const AbilityDefDto& dto,
		const DefKeyIndex<AbilityId>& abilityKeyIndex,
		std::span<const AttributeDef> attributes,
		std::span<const GameplayEffectDef> effects,
		std::span<const GameplayTagDef> tags,
		AbilityDef& outDef,
		std::string& outError)
	{
		if (!abilityKeyIndex.FindId(dto.key, outDef.id))
		{
			outError = "Unknown ability key: " + dto.key;
			return false;
		}

		outDef.key = dto.key;
		outDef.name = dto.name;
		outDef.kind = dto.kind;
		if (!CompileTagMask(
				std::span<const std::string>(
					dto.tagKeys.data(),
					dto.tagKeys.size()),
				tags,
				outDef.tags,
				outError) ||
			!CompileTagQuery(
				dto.activation.requiredOwnerTags,
				tags,
				outDef.activation.requiredOwnerTags,
				outError) ||
			!CompileTagQuery(
				dto.activation.blockedOwnerTags,
				tags,
				outDef.activation.blockedOwnerTags,
				outError))
		{
			return false;
		}

		outDef.activation.requiresTarget = dto.activation.requiresTarget;
		outDef.activation.requiresGrounded = dto.activation.requiresGrounded;

		if (!CompileVector<AbilityCostDto, AbilityCostDef>(
				std::span<const AbilityCostDto>(
					dto.costs.data(),
					dto.costs.size()),
				outDef.costs,
				[attributes](
					const AbilityCostDto& costDto,
					AbilityCostDef& costDef,
					std::string& error)
				{
					return CompileCost(costDto, attributes, costDef, error);
				},
				outError))
		{
			return false;
		}

		outDef.timeline.durationSec = dto.timeline.durationSec;
		outDef.timeline.policy = dto.timeline.policy;
		if (!CompileVector<AbilityCombatWindowDto, AbilityCombatWindowDef>(
				std::span<const AbilityCombatWindowDto>(
					dto.timeline.combatWindows.data(),
					dto.timeline.combatWindows.size()),
				outDef.timeline.combatWindows,
				[effects](
					const AbilityCombatWindowDto& windowDto,
					AbilityCombatWindowDef& windowDef,
					std::string& error)
				{
					return CompileCombatWindow(
						windowDto,
						effects,
						windowDef,
						error);
				},
				outError) ||
			!CompileVector<AbilityMovementSegmentDto, AbilityMovementSegmentDef>(
				std::span<const AbilityMovementSegmentDto>(
					dto.timeline.movementSegments.data(),
					dto.timeline.movementSegments.size()),
				outDef.timeline.movementSegments,
				CompileMovementSegment,
				outError) ||
			!CompileVector<AbilityEventDto, AbilityEventDef>(
				std::span<const AbilityEventDto>(
					dto.timeline.events.data(),
					dto.timeline.events.size()),
				outDef.timeline.events,
				[effects, tags](
					const AbilityEventDto& eventDto,
					AbilityEventDef& eventDef,
					std::string& error)
				{
					return CompileEvent(eventDto, effects, tags, eventDef, error);
				},
				outError))
		{
			return false;
		}

		outDef.transition.defaultNextAbilityId = InvalidAbilityId;
		if (dto.transition.defaultNextAbilityKey.has_value() &&
			!abilityKeyIndex.FindId(
				*dto.transition.defaultNextAbilityKey,
				outDef.transition.defaultNextAbilityId))
		{
			outError = "Unknown default next ability key: " +
				*dto.transition.defaultNextAbilityKey;
			return false;
		}

		if (!CompileEndPolicy(
				dto.transition.endPolicy,
				abilityKeyIndex,
				outDef.transition.endPolicy,
				outError))
		{
			return false;
		}

		return
			CompileVector<AbilityTransitionRuleDto, AbilityTransitionRuleDef>(
				std::span<const AbilityTransitionRuleDto>(
					dto.transition.interruptRules.data(),
					dto.transition.interruptRules.size()),
				outDef.transition.interruptRules,
				[&abilityKeyIndex](
					const AbilityTransitionRuleDto& ruleDto,
					AbilityTransitionRuleDef& ruleDef,
					std::string& error)
				{
					return CompileTransitionRule(
						ruleDto,
						abilityKeyIndex,
						ruleDef,
						error);
				},
				outError) &&
			CompileVector<AbilityTransitionRuleDto, AbilityTransitionRuleDef>(
				std::span<const AbilityTransitionRuleDto>(
					dto.transition.cancelRules.data(),
					dto.transition.cancelRules.size()),
				outDef.transition.cancelRules,
				[&abilityKeyIndex](
					const AbilityTransitionRuleDto& ruleDto,
					AbilityTransitionRuleDef& ruleDef,
					std::string& error)
				{
					return CompileTransitionRule(
						ruleDto,
						abilityKeyIndex,
						ruleDef,
						error);
				},
				outError);
	}
}

DefLoadResult LoadAbilityDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	std::span<const AttributeDef> attributes,
	std::span<const GameplayEffectDef> effects,
	std::span<const GameplayTagDef> tags,
	AbilityDefRegistry& outRegistry)
{
	DefLoadResult result{};
	std::vector<DefJsonDocument> documents;
	result = LoadDefJsonDocumentsFromDirectory(
		directory,
		documents,
		"Ability");
	if (!result.succeeded)
		return result;

	DefParsedDtoList<AbilityDefDto> dtos;
	if (!ParseDefDtosFromJsonDocuments(
			std::span<const DefJsonDocument>(documents.data(), documents.size()),
			AppendAbilityDocumentDtos,
			dtos,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	DefKeyIndex<AbilityId> abilityKeyIndex;
	if (!BuildAbilityKeyIndex(
			std::span<const DefParsedDto<AbilityDefDto>>(
				dtos.data(),
				dtos.size()),
			abilityKeyIndex,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	std::vector<AbilityDef> defs;
	if (!CompileRuntimeDefs<AbilityDefDto, AbilityDef>(
			std::span<const DefParsedDto<AbilityDefDto>>(
				dtos.data(),
				dtos.size()),
			[&abilityKeyIndex, attributes, effects, tags](
				const AbilityDefDto& dto,
				AbilityDef& outDef,
				std::string& outError)
			{
				return CompileAbilityDef(
					dto,
					abilityKeyIndex,
					attributes,
					effects,
					tags,
					outDef,
					outError);
			},
			defs,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	std::sort(
		defs.begin(),
		defs.end(),
		[](const AbilityDef& lhs, const AbilityDef& rhs)
		{
			return lhs.id < rhs.id;
		});

	if (!ValidateAbilityDefs(
			std::span<const AbilityDef>(defs.data(), defs.size()),
			attributes,
			effects,
			tags,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	AbilityDefRegistry registry;
	std::string registryError;
	if (!registry.Build(std::move(defs), &registryError))
	{
		result.succeeded = false;
		result.error = registryError;
		return result;
	}

	outRegistry = std::move(registry);
	result.succeeded = true;
	result.loadedCount = outRegistry.Size();
	result.error.clear();
	return result;
}
