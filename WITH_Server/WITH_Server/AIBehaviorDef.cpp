#include "pch.h"
#include "AIBehaviorDef.h"

#include "DefCompilePipeline.h"
#include "DefEnumString.h"
#include "DefJsonReader.h"
#include "GameplayContentCatalog.h"
#include "DefRegistry.h"
#include "json.hpp"

#include <algorithm>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

namespace
{
	struct AIBehaviorProfileDto
	{
		AIBehaviorProfileDef profile;
		std::vector<AIActionDef> combatActionDefs;
		std::vector<AIActionDef> idleActionDefs;
		std::vector<AIMovementProfileDef> movementProfiles;
		std::vector<AIReactionRuleDef> reactionRules;
		std::vector<AIPhaseTransitionDef> phaseTransitions;
		std::unordered_map<std::string, AIActionGroupId> actionGroupIds;
	};

	template<typename TEnum>
	bool ReadEnum(
		const json& node,
		const char* field,
		TEnum& outValue,
		const char* typeName,
		std::string& outError)
	{
		std::string text;
		if (!ReadRequiredString(node, field, text, outError))
			return false;

		if (!ParseDefString(text, outValue))
		{
			outError = std::string("Unknown ") + typeName + ": " + text;
			return false;
		}

		return true;
	}

	template<typename TEnum>
	bool ReadOptionalEnum(
		const json& node,
		const char* field,
		TEnum& outValue,
		const char* typeName,
		std::string& outError)
	{
		if (!node.contains(field))
			return true;

		std::string text;
		if (!ReadRequiredString(node, field, text, outError))
			return false;

		if (!ParseDefString(text, outValue))
		{
			outError = std::string("Unknown ") + typeName + ": " + text;
			return false;
		}

		return true;
	}

	bool ReadAIProfileKey(
		const json& node,
		const char* field,
		std::string& outKey,
		AIBehaviorProfileId& outId,
		std::string& outError)
	{
		if (!ReadRequiredString(node, field, outKey, outError))
			return false;

		outId = HashDefKey(outKey);
		if (outId == InvalidAIBehaviorProfileId)
		{
			outError = "Invalid AI profile key: " + outKey;
			return false;
		}

		return true;
	}

	bool ParsePerceptionTuning(
		const json& node,
		AIPerceptionTuningDef& outTuning,
		std::string& outError)
	{
		return
			ReadOptionalNumber(node, "sightRange", outTuning.sightRange, outError) &&
			ReadOptionalNumber(node, "attackRange", outTuning.attackRange, outError) &&
			ReadOptionalNumber(node, "combatExitRangeBonus", outTuning.combatExitRangeBonus, outError) &&
			ReadOptionalNumber(node, "frontDotThreshold", outTuning.frontDotThreshold, outError) &&
			ReadOptionalNumber(node, "targetKeepBonus", outTuning.targetKeepBonus, outError) &&
			ReadOptionalNumber(node, "lastAttackerBonus", outTuning.lastAttackerBonus, outError) &&
			ReadOptionalNumber(node, "frontBonus", outTuning.frontBonus, outError) &&
			ReadOptionalNumber(node, "switchScoreMargin", outTuning.switchScoreMargin, outError) &&
			ReadOptionalNumber(node, "attacksBeforeForcedRetarget", outTuning.attacksBeforeForcedRetarget, outError) &&
			ReadOptionalNumber(node, "loseSightGraceTime", outTuning.loseSightGraceTime, outError) &&
			ReadOptionalNumber(node, "leashRange", outTuning.leashRange, outError) &&
			ReadOptionalNumber(node, "hardLeashRange", outTuning.hardLeashRange, outError) &&
			ReadOptionalNumber(node, "leashGaugeMax", outTuning.leashGaugeMax, outError) &&
			ReadOptionalNumber(node, "leashDrainPerSec", outTuning.leashDrainPerSec, outError) &&
			ReadOptionalNumber(node, "hardLeashDrainPerSec", outTuning.hardLeashDrainPerSec, outError) &&
			ReadOptionalNumber(node, "leashRecoverPerSec", outTuning.leashRecoverPerSec, outError) &&
			ReadOptionalNumber(node, "returnHomeArriveRange", outTuning.returnHomeArriveRange, outError) &&
			ReadOptionalNumber(node, "returnHomeReaggroLockSec", outTuning.returnHomeReaggroLockSec, outError) &&
			ReadOptionalNumber(node, "returnHpRegenPerSecRatio", outTuning.returnHpRegenPerSecRatio, outError) &&
			ReadOptionalNumber(node, "idleActionCooldownSec", outTuning.idleActionCooldownSec, outError) &&
			ReadOptionalNumber(node, "idleActionChancePercent", outTuning.idleActionChancePercent, outError) &&
			ReadOptionalNumber(node, "assistRange", outTuning.assistRange, outError);
	}

	bool ReadOptionalBool(
		const json& node,
		const char* field,
		bool& outValue,
		std::string& outError);

	bool ParseTargetingTuning(
		const json& node,
		AITargetingTuningDef& outTuning,
		std::string& outError)
	{
		return
			ReadOptionalNumber(node, "targetKeepBonus", outTuning.targetKeepBonus, outError) &&
			ReadOptionalNumber(node, "lastAttackerBonus", outTuning.lastAttackerBonus, outError) &&
			ReadOptionalNumber(node, "frontBonus", outTuning.frontBonus, outError) &&
			ReadOptionalNumber(node, "switchScoreMargin", outTuning.switchScoreMargin, outError) &&
			ReadOptionalNumber(node, "attacksBeforeForcedRetarget", outTuning.attacksBeforeForcedRetarget, outError) &&
			ReadOptionalBool(node, "preferNearest", outTuning.preferNearest, outError) &&
			ReadOptionalBool(node, "preferLowestHp", outTuning.preferLowestHp, outError) &&
			ReadOptionalNumber(node, "assistRange", outTuning.assistRange, outError);
	}

	void NormalizeTargetingFallback(AIBehaviorProfileDef& profile) noexcept
	{
		profile.targeting.targetKeepBonus = profile.perception.targetKeepBonus;
		profile.targeting.lastAttackerBonus = profile.perception.lastAttackerBonus;
		profile.targeting.frontBonus = profile.perception.frontBonus;
		profile.targeting.switchScoreMargin = profile.perception.switchScoreMargin;
		profile.targeting.attacksBeforeForcedRetarget =
			profile.perception.attacksBeforeForcedRetarget;
		profile.targeting.assistRange = profile.perception.assistRange;
	}

	void MirrorTargetingToDeprecatedPerception(AIBehaviorProfileDef& profile) noexcept
	{
		profile.perception.targetKeepBonus = profile.targeting.targetKeepBonus;
		profile.perception.lastAttackerBonus = profile.targeting.lastAttackerBonus;
		profile.perception.frontBonus = profile.targeting.frontBonus;
		profile.perception.switchScoreMargin = profile.targeting.switchScoreMargin;
		profile.perception.attacksBeforeForcedRetarget =
			profile.targeting.attacksBeforeForcedRetarget;
		profile.perception.assistRange = profile.targeting.assistRange;
	}

	bool ParseDecisionTuning(
		const json& node,
		AIDecisionTuningDef& outTuning,
		std::string& outError)
	{
		return
			ReadOptionalNumber(node, "decisionInterval", outTuning.decisionInterval, outError) &&
			ReadOptionalNumber(node, "attackCooldown", outTuning.attackCooldown, outError) &&
			ReadOptionalNumber(node, "reactDuration", outTuning.reactDuration, outError);
	}

	bool ReadOptionalBool(
		const json& node,
		const char* field,
		bool& outValue,
		std::string& outError)
	{
		if (!node.contains(field))
			return true;

		if (!node.at(field).is_boolean())
		{
			outError = std::string("Invalid bool field: ") + field;
			return false;
		}

		outValue = node.at(field).get<bool>();
		return true;
	}

	bool ReadOptionalAbilityId(
		const json& node,
		const char* field,
		AbilityId& outAbilityId,
		std::string& outError)
	{
		if (!node.contains(field) || node.at(field).is_null())
			return true;

		std::string abilityKey;
		if (!ReadRequiredString(node, field, abilityKey, outError))
			return false;

		const GameplayContentCatalogSnapshot* catalog =
			GameplayContentCatalogSnapshot::TryCurrent();
		const AbilityDef* ability =
			catalog != nullptr ? catalog->FindAbilityByKey(abilityKey) : nullptr;
		if (ability == nullptr)
		{
			outError = "Unknown ability key: " + abilityKey;
			return false;
		}

		outAbilityId = ability->id;
		return true;
	}

	AIActionGroupId ResolveActionGroupId(
		const std::string& groupName,
		std::unordered_map<std::string, AIActionGroupId>& groupIds)
	{
		const auto it = groupIds.find(groupName);
		if (it != groupIds.end())
			return it->second;

		const AIActionGroupId id =
			static_cast<AIActionGroupId>(groupIds.size() + 1u);
		groupIds.emplace(groupName, id);
		return id;
	}

	bool ReadOptionalFloatField(
		const json& node,
		const char* field,
		std::optional<float>& outValue,
		std::string& outError)
	{
		if (!node.contains(field) || node.at(field).is_null())
			return true;

		if (!node.at(field).is_number())
		{
			outError = std::string("Invalid numeric field: ") + field;
			return false;
		}

		outValue = node.at(field).get<float>();
		return true;
	}

	bool ParseActionList(
		const json& node,
		std::vector<AIActionDef>& outActions,
		std::unordered_map<std::string, AIActionGroupId>& groupIds,
		float defaultGlobalCooldownSec,
		bool idleDefaults,
		std::string& outError)
	{
		if (!node.is_array())
		{
			outError = "AI action list must be an array.";
			return false;
		}

		outActions.clear();
		outActions.reserve(node.size());
		for (const json& abilityNode : node)
		{
			AIActionDef action{};
			action.globalCooldownSec = defaultGlobalCooldownSec;
			if (idleDefaults)
			{
				action.globalCooldownSec = 0.0f;
				action.condition.requiresTargetVisible = false;
				action.condition.requiresTargetInFront = false;
			}

			if (!ReadOptionalAbilityId(
				abilityNode,
				"abilityKey",
				action.abilityId,
				outError))
			{
				return false;
			}
			if (action.abilityId == InvalidAbilityId)
			{
				outError = "AI action requires abilityKey.";
				return false;
			}

			if (!ReadOptionalNumber(abilityNode, "weight", action.weight, outError) ||
				!ReadOptionalNumber(abilityNode, "phaseMin", action.condition.phaseMin, outError) ||
				!ReadOptionalNumber(abilityNode, "phaseMax", action.condition.phaseMax, outError) ||
				!ReadOptionalEnum(
					abilityNode,
					"distanceBucket",
					action.condition.distanceBucket,
					"AI action distance bucket",
					outError) ||
				!ReadOptionalFloatField(abilityNode, "minDistance", action.condition.minDistance, outError) ||
				!ReadOptionalFloatField(abilityNode, "maxDistance", action.condition.maxDistance, outError) ||
				!ReadOptionalNumber(abilityNode, "selfHpRatioMin", action.condition.selfHpRatioMin, outError) ||
				!ReadOptionalNumber(abilityNode, "selfHpRatioMax", action.condition.selfHpRatioMax, outError) ||
				!ReadOptionalNumber(abilityNode, "targetHpRatioMin", action.condition.targetHpRatioMin, outError) ||
				!ReadOptionalNumber(abilityNode, "targetHpRatioMax", action.condition.targetHpRatioMax, outError) ||
				!ReadOptionalBool(abilityNode, "requiresTargetVisible", action.condition.requiresTargetVisible, outError) ||
				!ReadOptionalBool(abilityNode, "requiresTargetInFront", action.condition.requiresTargetInFront, outError) ||
				!ReadOptionalBool(abilityNode, "requiresAbilityAvailable", action.condition.requiresAbilityAvailable, outError) ||
				!ReadOptionalNumber(abilityNode, "aiCooldownSec", action.aiCooldownSec, outError) ||
				!ReadOptionalNumber(abilityNode, "globalCooldownSec", action.globalCooldownSec, outError) ||
				!ReadOptionalNumber(abilityNode, "groupCooldownSec", action.groupCooldownSec, outError) ||
				!ReadOptionalBool(abilityNode, "forbidImmediateRepeat", action.forbidImmediateRepeat, outError) ||
				!ReadOptionalNumber(abilityNode, "repeatWeightMultiplier", action.repeatWeightMultiplier, outError) ||
				!ReadOptionalNumber(abilityNode, "lockMovementSec", action.lockMovementSec, outError) ||
				!ReadOptionalBool(abilityNode, "lockFacingToTarget", action.lockFacingToTarget, outError) ||
				!ReadOptionalNumber(abilityNode, "chancePercent", action.chancePercent, outError) ||
				!ReadOptionalEnum(
					abilityNode,
					"actionRole",
					action.actionRole,
					"AI action role",
					outError) ||
				!ReadOptionalNumber(abilityNode, "requiresBasicActionCount", action.requiresBasicActionCount, outError) ||
				!ReadOptionalBool(abilityNode, "resetsBasicActionCount", action.resetsBasicActionCount, outError))
			{
				return false;
			}

			if (abilityNode.contains("patternGroup") &&
				!abilityNode.at("patternGroup").is_null())
			{
				std::string patternGroup;
				if (!ReadRequiredString(
					abilityNode,
					"patternGroup",
					patternGroup,
					outError))
				{
					return false;
				}

				action.groupId = ResolveActionGroupId(patternGroup, groupIds);
			}

			outActions.push_back(action);
		}

		return true;
	}

	AIMovementProfileDef MakeDefaultMovementProfile(
		const AIPerceptionTuningDef& perception)
	{
		const double attackRange = std::max(0.1, perception.attackRange);
		AIMovementProfileDef profile{};
		profile.name = "Default";
		profile.veryCloseDistance = attackRange * 0.35;
		profile.closeDistance = attackRange;
		profile.midDistance = attackRange * 1.6;
		profile.farDistance = attackRange * 2.5;
		profile.preferredMinDistance = attackRange * 0.45;
		profile.preferredMaxDistance = attackRange * 0.9;
		return profile;
	}

	bool ParseMovementProfiles(
		const json& node,
		const AIPerceptionTuningDef& perception,
		std::vector<AIMovementProfileDef>& outProfiles,
		std::string& outError)
	{
		if (!node.is_array())
		{
			outError = "movementProfiles must be an array.";
			return false;
		}

		outProfiles.clear();
		outProfiles.reserve(node.size());
		for (const json& profileNode : node)
		{
			AIMovementProfileDef profile = MakeDefaultMovementProfile(perception);

			if (profileNode.contains("name") &&
				!profileNode.at("name").is_null())
			{
				if (!ReadRequiredString(profileNode, "name", profile.name, outError))
					return false;
			}

			if (!ReadOptionalNumber(profileNode, "phaseMin", profile.phaseMin, outError) ||
				!ReadOptionalNumber(profileNode, "phaseMax", profile.phaseMax, outError) ||
				!ReadOptionalNumber(profileNode, "veryCloseDistance", profile.veryCloseDistance, outError) ||
				!ReadOptionalNumber(profileNode, "closeDistance", profile.closeDistance, outError) ||
				!ReadOptionalNumber(profileNode, "midDistance", profile.midDistance, outError) ||
				!ReadOptionalNumber(profileNode, "farDistance", profile.farDistance, outError) ||
				!ReadOptionalNumber(profileNode, "preferredMinDistance", profile.preferredMinDistance, outError) ||
				!ReadOptionalNumber(profileNode, "preferredMaxDistance", profile.preferredMaxDistance, outError) ||
				!ReadOptionalNumber(profileNode, "distanceHysteresis", profile.distanceHysteresis, outError) ||
				!ReadOptionalEnum(profileNode, "veryCloseBehavior", profile.veryCloseBehavior, "AI movement behavior", outError) ||
				!ReadOptionalEnum(profileNode, "closeBehavior", profile.closeBehavior, "AI movement behavior", outError) ||
				!ReadOptionalEnum(profileNode, "preferredBehavior", profile.preferredBehavior, "AI movement behavior", outError) ||
				!ReadOptionalEnum(profileNode, "midBehavior", profile.midBehavior, "AI movement behavior", outError) ||
				!ReadOptionalEnum(profileNode, "farBehavior", profile.farBehavior, "AI movement behavior", outError) ||
				!ReadOptionalNumber(profileNode, "strafeMinSec", profile.strafeMinSec, outError) ||
				!ReadOptionalNumber(profileNode, "strafeMaxSec", profile.strafeMaxSec, outError) ||
				!ReadOptionalNumber(profileNode, "strafeChangeChancePercent", profile.strafeChangeChancePercent, outError) ||
				!ReadOptionalBool(profileNode, "allowNavPathing", profile.allowNavPathing, outError) ||
				!ReadOptionalBool(profileNode, "lockFacingToTarget", profile.lockFacingToTarget, outError))
			{
				return false;
			}

			outProfiles.push_back(std::move(profile));
		}

		return true;
	}

	bool ParsePhaseTransitions(
		const json& node,
		std::vector<AIPhaseTransitionDef>& outTransitions,
		std::string& outError)
	{
		if (!node.is_array())
		{
			outError = "phaseTransitions must be an array.";
			return false;
		}

		outTransitions.clear();
		outTransitions.reserve(node.size());
		for (const json& transitionNode : node)
		{
			AIPhaseTransitionDef transition{};
			if (transitionNode.contains("fromPhase") &&
				!transitionNode.at("fromPhase").is_null())
			{
				uint8_t fromPhase{ 1 };
				if (!ReadRequiredNumber(
					transitionNode,
					"fromPhase",
					fromPhase,
					outError))
				{
					return false;
				}
				transition.fromPhase = fromPhase;
			}

			if (transitionNode.contains("toPhase"))
			{
				if (!ReadRequiredNumber(
					transitionNode,
					"toPhase",
					transition.toPhase,
					outError))
				{
					return false;
				}
			}
			else
			{
				outError = "phaseTransitions entry requires toPhase.";
				return false;
			}

			if (!ReadRequiredNumber(transitionNode, "hpRatio", transition.hpRatio, outError) ||
				!ReadOptionalAbilityId(
					transitionNode,
					"transitionAbilityKey",
					transition.transitionAbilityId,
					outError) ||
				!ReadOptionalNumber(transitionNode, "transitionLockSec", transition.transitionLockSec, outError) ||
				!ReadOptionalBool(transitionNode, "clearActionCooldowns", transition.clearActionCooldowns, outError) ||
				!ReadOptionalBool(transitionNode, "clearGroupCooldowns", transition.clearGroupCooldowns, outError) ||
				!ReadOptionalBool(transitionNode, "forceRetarget", transition.forceRetarget, outError))
			{
				return false;
			}

			outTransitions.push_back(transition);
		}

		return true;
	}

	bool ParseReactionRules(
		const json& node,
		std::vector<AIReactionRuleDef>& outRules,
		std::string& outError)
	{
		if (!node.is_array())
		{
			outError = "reactionRules must be an array.";
			return false;
		}

		outRules.clear();
		outRules.reserve(node.size());
		for (const json& ruleNode : node)
		{
			AIReactionRuleDef rule{};
			if (!ReadEnum(ruleNode, "event", rule.event, "AI reaction rule event", outError) ||
				!ReadOptionalNumber(ruleNode, "phaseMin", rule.phaseMin, outError) ||
				!ReadOptionalNumber(ruleNode, "phaseMax", rule.phaseMax, outError) ||
				!ReadOptionalNumber(ruleNode, "selfHpRatioMin", rule.selfHpRatioMin, outError) ||
				!ReadOptionalNumber(ruleNode, "selfHpRatioMax", rule.selfHpRatioMax, outError) ||
				!ReadEnum(ruleNode, "outcome", rule.outcome, "AI reaction rule outcome", outError) ||
				!ReadOptionalBool(ruleNode, "retargetAttacker", rule.retargetAttacker, outError) ||
				!ReadOptionalAbilityId(ruleNode, "reactAbilityKey", rule.reactAbilityId, outError) ||
				!ReadOptionalNumber(ruleNode, "reactDurationSec", rule.reactDurationSec, outError) ||
				!ReadOptionalNumber(ruleNode, "priority", rule.priority, outError))
			{
				return false;
			}

			outRules.push_back(rule);
		}

		return true;
	}

	void AppendDefaultReactionRules(
		AIArchetype aiType,
		std::vector<AIReactionRuleDef>& outRules)
	{
		if (!outRules.empty())
			return;

		const auto MakeRule =
			[](AIReactionRuleEvent event,
				AIReactionRuleOutcome outcome,
				bool retarget,
				int priority) noexcept
			{
				AIReactionRuleDef rule{};
				rule.event = event;
				rule.outcome = outcome;
				rule.retargetAttacker = retarget;
				rule.priority = priority;
				return rule;
			};

		switch (aiType) {
		case AIArchetype::FirstBossMonster:
		case AIArchetype::MidBossMonster:
		case AIArchetype::FinalBossMonster:
			outRules.push_back(MakeRule(
				AIReactionRuleEvent::OnHitReceived,
				AIReactionRuleOutcome::ForceRetarget,
				true,
				100));
			outRules.push_back(MakeRule(
				AIReactionRuleEvent::OnParried,
				AIReactionRuleOutcome::EnterReact,
				true,
				200));
			outRules.push_back(MakeRule(
				AIReactionRuleEvent::OnGuardBroken,
				AIReactionRuleOutcome::EnterReact,
				true,
				200));
			break;
		case AIArchetype::NormalMonster:
			outRules.push_back(MakeRule(
				AIReactionRuleEvent::OnHitReceived,
				AIReactionRuleOutcome::EnterReact,
				true,
				100));
			outRules.push_back(MakeRule(
				AIReactionRuleEvent::OnParried,
				AIReactionRuleOutcome::EnterReact,
				true,
				200));
			outRules.push_back(MakeRule(
				AIReactionRuleEvent::OnGuardBroken,
				AIReactionRuleOutcome::EnterReact,
				false,
				200));
			break;
		default:
			break;
		}
	}

	bool ParseProfileDocument(
		const json& root,
		AIBehaviorProfileDef& outProfile,
		std::vector<AIActionDef>& outCombatActionDefs,
		std::vector<AIActionDef>& outIdleActionDefs,
		std::vector<AIMovementProfileDef>& outMovementProfiles,
		std::vector<AIReactionRuleDef>& outReactionRules,
		std::vector<AIPhaseTransitionDef>& outPhaseTransitions,
		std::unordered_map<std::string, AIActionGroupId>& actionGroupIds,
		std::string& outError)
	{
		if (!ReadAIProfileKey(root, "key", outProfile.key, outProfile.id, outError) ||
			!ReadEnum(root, "aiType", outProfile.aiType, "AI archetype", outError))
		{
			return false;
		}

		if (root.contains("bossPattern"))
		{
			outError =
				"bossPattern is no longer supported. Use movementProfiles and phaseTransitions.";
			return false;
		}

		if (root.contains("perceptionTuning") &&
			!ParsePerceptionTuning(
				root.at("perceptionTuning"),
				outProfile.perception,
				outError))
		{
			return false;
		}
		NormalizeTargetingFallback(outProfile);

		if (root.contains("targeting") &&
			!ParseTargetingTuning(
				root.at("targeting"),
				outProfile.targeting,
				outError))
		{
			return false;
		}
		MirrorTargetingToDeprecatedPerception(outProfile);

		if (root.contains("decisionTuning") &&
			!ParseDecisionTuning(
				root.at("decisionTuning"),
				outProfile.decision,
				outError))
		{
			return false;
		}

		if (root.contains("combatActions") &&
			!ParseActionList(
				root.at("combatActions"),
				outCombatActionDefs,
				actionGroupIds,
				static_cast<float>(outProfile.decision.attackCooldown),
				false,
				outError))
		{
			return false;
		}

		if (root.contains("idleActions") &&
			!ParseActionList(
				root.at("idleActions"),
				outIdleActionDefs,
				actionGroupIds,
				0.0f,
				true,
				outError))
		{
			return false;
		}

		if (root.contains("movementProfiles") &&
			!ParseMovementProfiles(
				root.at("movementProfiles"),
				outProfile.perception,
				outMovementProfiles,
				outError))
		{
			return false;
		}

		if (root.contains("reactionRules") &&
			!ParseReactionRules(root.at("reactionRules"), outReactionRules, outError))
		{
			return false;
		}

		if (root.contains("phaseTransitions"))
		{
			if (!ParsePhaseTransitions(
					root.at("phaseTransitions"),
					outPhaseTransitions,
					outError))
			{
				return false;
			}
		}

		if (outMovementProfiles.empty())
		{
			outMovementProfiles.push_back(
				MakeDefaultMovementProfile(outProfile.perception));
		}
		AppendDefaultReactionRules(outProfile.aiType, outReactionRules);

		return true;
	}

	bool ValidateLoadedProfiles(
		std::span<const AIBehaviorProfileDef> profiles,
		std::string& outError)
	{
		for (size_t i = 0; i < profiles.size(); ++i)
		{
			const AIBehaviorProfileDef& profile = profiles[i];
			if (profile.id == InvalidAIBehaviorProfileId)
			{
				outError = "AI behavior profile key must not be empty.";
				return false;
			}

			for (size_t j = i + 1; j < profiles.size(); ++j)
			{
				const AIBehaviorProfileDef& other = profiles[j];
				if (profile.id == other.id)
				{
					if (profile.key != other.key)
					{
						outError =
							"AI behavior profile key hash collision: " +
							profile.key + " / " + other.key;
					}
					else
					{
						outError = "Duplicate AI behavior profile key: " + profile.key;
					}
					return false;
				}
			}

			if (profile.combatActionDefs.empty())
			{
				outError = "AI behavior profile requires combatActions.";
				return false;
			}
			if (profile.perception.attackRange < 0.0 ||
				profile.perception.combatExitRangeBonus < 0.0)
			{
				outError =
					"AI behavior perception has negative combat range tuning.";
				return false;
			}
			if (profile.targeting.targetKeepBonus < 0.0 ||
				profile.targeting.lastAttackerBonus < 0.0 ||
				profile.targeting.frontBonus < 0.0 ||
				profile.targeting.switchScoreMargin < 0.0 ||
				profile.targeting.attacksBeforeForcedRetarget < 0)
			{
				outError =
					"AI behavior targeting has negative tuning.";
				return false;
			}

			for (const AIActionDef& action : profile.combatActionDefs)
			{
				if (action.weight == 0)
				{
					outError = "AI behavior combatActions requires positive weight.";
					return false;
				}
				if (action.condition.phaseMin > action.condition.phaseMax)
				{
					outError = "AI behavior combatActions has invalid phase range.";
					return false;
				}
				if (action.condition.selfHpRatioMin > action.condition.selfHpRatioMax ||
					action.condition.targetHpRatioMin > action.condition.targetHpRatioMax)
				{
					outError = "AI behavior combatActions has invalid hp ratio range.";
					return false;
				}
				if (action.condition.minDistance.has_value() &&
					action.condition.maxDistance.has_value() &&
					*action.condition.minDistance >
						*action.condition.maxDistance)
				{
					outError =
						"AI behavior combatActions has invalid distance range.";
					return false;
				}
				if (action.aiCooldownSec < 0.0f ||
					action.globalCooldownSec < 0.0f ||
					action.groupCooldownSec < 0.0f ||
					action.lockMovementSec < 0.0f)
				{
					outError = "AI behavior combatActions has negative cooldown or lock.";
					return false;
				}
				if (action.requiresBasicActionCount > 0 &&
					action.actionRole != AIActionRole::Effect)
				{
					outError = "AI behavior combatActions basic-count requirement is only valid for effect actions.";
					return false;
				}
				if (action.resetsBasicActionCount &&
					action.actionRole != AIActionRole::Effect)
				{
					outError = "AI behavior combatActions basic-count reset is only valid for effect actions.";
					return false;
				}
			}

			for (const AIActionDef& action : profile.idleActionDefs)
			{
				if (action.weight == 0)
				{
					outError = "AI behavior idleActions requires positive weight.";
					return false;
				}
				if (action.aiCooldownSec < 0.0f ||
					action.globalCooldownSec < 0.0f ||
					action.groupCooldownSec < 0.0f ||
					action.lockMovementSec < 0.0f)
				{
					outError = "AI behavior idleActions has negative cooldown or lock.";
					return false;
				}
			}

			for (const AIMovementProfileDef& movement : profile.movementProfiles)
			{
				if (movement.phaseMin > movement.phaseMax)
				{
					outError = "AI behavior movementProfiles has invalid phase range.";
					return false;
				}
				if (movement.strafeMinSec < 0.0f ||
					movement.strafeMaxSec < 0.0f ||
					movement.strafeMinSec > movement.strafeMaxSec)
				{
					outError = "AI behavior movementProfiles has invalid strafe range.";
					return false;
				}
				if (movement.veryCloseDistance < 0.0 ||
					movement.closeDistance < movement.veryCloseDistance ||
					movement.midDistance < movement.closeDistance ||
					movement.farDistance < movement.midDistance ||
					movement.preferredMinDistance < 0.0 ||
					movement.preferredMinDistance >
						movement.preferredMaxDistance ||
					movement.distanceHysteresis < 0.0)
				{
					outError =
						"AI behavior movementProfiles has invalid distance tuning.";
					return false;
				}
			}

			for (const AIReactionRuleDef& rule : profile.reactionRules)
			{
				if (rule.phaseMin > rule.phaseMax)
				{
					outError = "AI behavior reactionRules has invalid phase range.";
					return false;
				}
				if (rule.selfHpRatioMin > rule.selfHpRatioMax)
				{
					outError = "AI behavior reactionRules has invalid hp ratio range.";
					return false;
				}
				if (rule.reactDurationSec < 0.0f)
				{
					outError = "AI behavior reactionRules has negative reactDurationSec.";
					return false;
				}
			}

			for (const AIPhaseTransitionDef& transition : profile.phaseTransitions)
			{
				if (transition.toPhase < 1)
				{
					outError = "AI behavior phaseTransitions requires toPhase >= 1.";
					return false;
				}
				if (transition.hpRatio < 0.0f || transition.hpRatio > 1.0f)
				{
					outError = "AI behavior phaseTransitions hpRatio must be 0..1.";
					return false;
				}
				if (transition.transitionLockSec < 0.0f)
				{
					outError = "AI behavior phaseTransitions has negative transitionLockSec.";
					return false;
				}
			}
		}

		return true;
	}

	bool AppendAIBehaviorDocumentDtos(
		const json& root,
		std::vector<AIBehaviorProfileDto>& outDtos,
		std::string& outError)
	{
		AIBehaviorProfileDto dto{};
		if (!ParseProfileDocument(
				root,
				dto.profile,
				dto.combatActionDefs,
				dto.idleActionDefs,
				dto.movementProfiles,
				dto.reactionRules,
				dto.phaseTransitions,
				dto.actionGroupIds,
				outError))
		{
			return false;
		}

		outDtos.push_back(std::move(dto));
		return true;
	}

	bool BuildAIBehaviorDefinitionSet(
		std::span<const DefParsedDto<AIBehaviorProfileDto>> dtos,
		AIBehaviorDefinitionSet& outDefs,
		std::string& outError)
	{
		std::vector<AIBehaviorProfileDef> profiles;
		profiles.reserve(dtos.size());

		outDefs.combatActionDefStorage.clear();
		outDefs.idleActionDefStorage.clear();
		outDefs.movementProfileStorage.clear();
		outDefs.reactionRuleStorage.clear();
		outDefs.phaseTransitionStorage.clear();
		outDefs.combatActionDefStorage.reserve(dtos.size());
		outDefs.idleActionDefStorage.reserve(dtos.size());
		outDefs.movementProfileStorage.reserve(dtos.size());
		outDefs.reactionRuleStorage.reserve(dtos.size());
		outDefs.phaseTransitionStorage.reserve(dtos.size());

		for (const DefParsedDto<AIBehaviorProfileDto>& parsedDto : dtos)
		{
			profiles.push_back(parsedDto.dto.profile);
			outDefs.combatActionDefStorage.push_back(
				parsedDto.dto.combatActionDefs);
			outDefs.idleActionDefStorage.push_back(
				parsedDto.dto.idleActionDefs);
			outDefs.movementProfileStorage.push_back(
				parsedDto.dto.movementProfiles);
			outDefs.reactionRuleStorage.push_back(parsedDto.dto.reactionRules);
			outDefs.phaseTransitionStorage.push_back(
				parsedDto.dto.phaseTransitions);
		}

		for (size_t i = 0; i < profiles.size(); ++i)
		{
			profiles[i].combatActionDefs = std::span<const AIActionDef>(
				outDefs.combatActionDefStorage[i].data(),
				outDefs.combatActionDefStorage[i].size());
			profiles[i].idleActionDefs = std::span<const AIActionDef>(
				outDefs.idleActionDefStorage[i].data(),
				outDefs.idleActionDefStorage[i].size());
			profiles[i].movementProfiles = std::span<const AIMovementProfileDef>(
				outDefs.movementProfileStorage[i].data(),
				outDefs.movementProfileStorage[i].size());
			profiles[i].reactionRules = std::span<const AIReactionRuleDef>(
				outDefs.reactionRuleStorage[i].data(),
				outDefs.reactionRuleStorage[i].size());
			profiles[i].phaseTransitions = std::span<const AIPhaseTransitionDef>(
				outDefs.phaseTransitionStorage[i].data(),
				outDefs.phaseTransitionStorage[i].size());
		}

		if (!ValidateLoadedProfiles(
				std::span<const AIBehaviorProfileDef>(
					profiles.data(),
					profiles.size()),
				outError))
		{
			return false;
		}

		std::string registryError;
		if (!outDefs.profiles.Build(std::move(profiles), &registryError))
		{
			outError = registryError;
			return false;
		}

		return true;
	}
}

AIBehaviorDefLoadResult LoadAIBehaviorProfileDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	AIBehaviorDefinitionSet& outDefs)
{
	std::vector<DefJsonDocument> documents;
	AIBehaviorDefLoadResult result =
		LoadDefJsonDocumentsFromDirectory(directory, documents, "AI behavior");
	if (!result.succeeded)
		return result;

	DefParsedDtoList<AIBehaviorProfileDto> dtos;
	if (!ParseDefDtosFromJsonDocuments(
			std::span<const DefJsonDocument>(
				documents.data(),
				documents.size()),
			AppendAIBehaviorDocumentDtos,
			dtos,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	if (!BuildAIBehaviorDefinitionSet(
			std::span<const DefParsedDto<AIBehaviorProfileDto>>(
				dtos.data(),
				dtos.size()),
			outDefs,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	result.succeeded = true;
	result.loadedCount = outDefs.profiles.Size();
	result.error.clear();
	return result;
}
