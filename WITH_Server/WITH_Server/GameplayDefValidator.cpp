#include "pch.h"
#include "GameplayDefValidator.h"

#include "ActionDef.h"
#include "AIFSMRegistry.h"
#include "AIBehaviorDef.h"
#include "BuffDef.h"
#include "CharacterDef.h"
#include "SpawnSetDef.h"

#include <string>

DefLoadResult GamePlayDefValidator::ValidateGameplayDefs()
{
	DefLoadResult result{};

	if (!ValidateActionDefs(result.error))
		return result;

	if (!ValidateAIBehaviorProfiles(result.error))
		return result;

	if (!ValidateCharacterDefs(result.error))
		return result;

	if (!ValidateBuffDefs(result.error))
		return result;

	if (!ValidateSpawnSetDefs(result.error))
		return result;

	result.succeeded = true;
	result.loadedCount =
		GetActionDefs().size() + GetAIBehaviorProfileDefs().size() +
		GetCharacterDefs().size() + GetBuffDefs().size() + GetSpawnSetDefs().size();
	return result;
}


bool GamePlayDefValidator::ValidateWeightedAction(
	const WeightedActionEntry& entry,
	const char* owner,
	std::string& outError)
{
	if (entry.actionId == ActionId::None ||
		FindActionDef(entry.actionId) == nullptr)
	{
		outError = std::string(owner) +
			" references unknown weighted action.";
		return false;
	}

	if (entry.weight == 0)
	{
		outError = std::string(owner) +
			" has weighted action with zero weight.";
		return false;
	}

	return true;
}

bool GamePlayDefValidator::ValidateAIBehaviorProfiles(std::string& outError)
{
	for (const AIBehaviorProfileDef& profile : GetAIBehaviorProfileDefs())
	{
		if (profile.id == AITuningIds::None)
		{
			outError = "AI behavior profile id must not be None.";
			return false;
		}

		if (!AIFSMRegistry::IsArchetypeSupported(profile.aiType))
		{
			outError = "AI behavior profile uses unsupported AI archetype.";
			return false;
		}

		for (const WeightedActionEntry& entry : profile.combatActions)
		{
			if (!ValidateWeightedAction(
				entry,
				"AI behavior combatActions",
				outError))
			{
				return false;
			}
		}

		for (const WeightedActionEntry& entry : profile.idleActions)
		{
			if (!ValidateWeightedAction(
				entry,
				"AI behavior idleActions",
				outError))
			{
				return false;
			}
		}
	}

	return true;
}

bool GamePlayDefValidator::ValidateActionDefs(std::string& outError)
{
	for (const ActionDef& def : GetActionDefs())
	{
		if (def.id == ActionId::None)
		{
			outError = "Action id must not be None.";
			return false;
		}

		if (def.name.empty())
		{
			outError = "Action name must not be empty.";
			return false;
		}
	}

	for (const CharacterActionProfileDef& profile :
		GetCharacterActionProfileDefs())
	{
		for (const ActionId actionId : profile.availableActions)
		{
			if (FindActionDef(actionId) == nullptr)
			{
				outError =
					"Character action profile references unknown action.";
				return false;
			}
		}

		if (FindActionInputBindingProfileDef(profile.inputBindingProfileId) ==
				nullptr ||
			FindActionFallbackReactionProfileDef(
				profile.fallbackReactionProfileId) == nullptr ||
			FindAnimationBindingProfileDef(profile.animationBindingProfileId) ==
				nullptr)
		{
			outError =
				"Character action profile references missing sub-profile.";
			return false;
		}
	}

	return true;
}

bool GamePlayDefValidator::ValidateCharacterDefs(std::string& outError)
{
	for (const CharacterDef& def : GetCharacterDefs())
	{
		if (def.id == CharacterId::None)
		{
			outError = "Character id must not be None.";
			return false;
		}

		if (def.name.empty())
		{
			outError = "Character name must not be empty.";
			return false;
		}

		if (def.HasFeature(CharacterFeatureFlags::Combatant) && !def.action)
		{
			outError = "Combatant character requires action.";
			return false;
		}

		if (def.action)
		{
			const CharacterActionProfileDef* actionProfile =
				FindCharacterActionProfileDef(def.action->actionProfileId);
			if (actionProfile == nullptr)
			{
				outError = "Character references unknown action profile.";
				return false;
			}

			if (actionProfile->characterId != def.id)
			{
				outError =
					"Character action profile characterId does not match character.";
				return false;
			}
		}

		if (def.IsAIControlled())
		{
			if (!def.ai)
			{
				outError = "AIControlled character requires ai.";
				return false;
			}

			if (!AIFSMRegistry::IsArchetypeSupported(def.ai->aiType))
			{
				outError = "Character uses unsupported AI archetype.";
				return false;
			}

			if (def.ai->aiTuningId)
			{
				const AIBehaviorProfileDef* profile =
					FindAIBehaviorProfileDef(
						def.ai->aiType,
						*def.ai->aiTuningId);
				if (profile == nullptr)
				{
					outError =
						"Character references missing AI behavior profile.";
					return false;
				}

				if (!AIFSMRegistry::IsBehaviorSupported(
					def.ai->aiType,
					*def.ai->aiTuningId))
				{
					outError =
						"Character references unsupported AI behavior.";
					return false;
				}
			}
		}
		else if (def.ai)
		{
			outError = "Non-AIControlled character must not define ai.";
			return false;
		}
	}

	return true;
}

bool GamePlayDefValidator::ValidateBuffDefs(std::string& outError)
{
	for (const BuffDef& def : GetBuffDefs())
	{
		if (def.profile.id == BuffId::None)
		{
			outError = "Buff id must not be None.";
			return false;
		}

		if (def.profile.name.empty())
		{
			outError = "Buff name must not be empty.";
			return false;
		}

		for (const BuffRemoveRuleDef& rule : def.removeRules)
		{
			if (rule.operand.actionIdCondition &&
				FindActionDef(*rule.operand.actionIdCondition) == nullptr)
			{
				outError = "Buff remove rule references unknown action.";
				return false;
			}
		}
	}

	return true;
}

bool GamePlayDefValidator::ValidateSpawnSetDefs(std::string& outError)
{
	for (const SpawnSetDef& def : GetSpawnSetDefs())
	{
		if (def.id == SpawnSetId::None)
		{
			outError = "SpawnSet id must not be None.";
			return false;
		}

		if (def.name.empty())
		{
			outError = "SpawnSet name must not be empty.";
			return false;
		}

		for (const SpawnEntryDef& entry : def.entries)
		{
			if (FindCharacterDef(entry.characterId) == nullptr)
			{
				outError = "SpawnSet references unknown character.";
				return false;
			}
		}
	}

	return true;
}
