#include "pch.h"
#include "GameplayDefValidator.h"

#include "AIFSMRegistry.h"
#include "AIBehaviorDef.h"
#include "CharacterDef.h"
#include "GameDataCatalog.h"
#include "GameplayContentCatalog.h"
#include "SpawnSetDef.h"

#include <string>

DefLoadResult GamePlayDefValidator::ValidateGameplayDefs(
	const GameDataCatalog& catalog)
{
	DefLoadResult result{};

	if (!ValidateAIBehaviorProfiles(catalog, result.error))
		return result;

	if (!ValidateCharacterDefs(catalog, result.error))
		return result;

	if (!ValidateSpawnSetDefs(catalog, result.error))
		return result;

	result.succeeded = true;
	result.loadedCount =
		catalog.AIBehaviors().GetAll().size() +
		catalog.Characters().GetAll().size() +
		catalog.SpawnSets().GetAll().size();
	return result;
}


bool GamePlayDefValidator::ValidateWeightedAction(
	const WeightedActionEntry& entry,
	const char* owner,
	const GameDataCatalog&,
	std::string& outError)
{
	const GameplayContentCatalogSnapshot* content =
		GameplayContentCatalogSnapshot::TryCurrent();
	if (content == nullptr)
	{
		outError = "Gameplay content catalog is not published.";
		return false;
	}

	if (entry.abilityId == InvalidAbilityId ||
		content->Abilities().Find(entry.abilityId) == nullptr)
	{
		outError = std::string(owner) +
			" references unknown weighted ability.";
		return false;
	}

	if (entry.weight == 0)
	{
		outError = std::string(owner) +
			" has weighted ability with zero weight.";
		return false;
	}

	return true;
}

bool GamePlayDefValidator::ValidateAIBehaviorProfiles(
	const GameDataCatalog& catalog,
	std::string& outError)
{
	for (const AIBehaviorProfileDef& profile :
		catalog.AIBehaviors().GetAll())
	{
		if (profile.id == InvalidAIBehaviorProfileId)
		{
			outError = "AI behavior profile key must not be empty.";
			return false;
		}

		if (!AIFSMRegistry::IsArchetypeSupported(profile.aiType))
		{
			outError = "AI behavior profile uses unsupported AI archetype.";
			return false;
		}

		if (!AIFSMRegistry::IsBehaviorProfileSupported(profile))
		{
			outError = "AI behavior profile uses unsupported archetype.";
			return false;
		}

		for (const WeightedActionEntry& entry : profile.combatActions)
		{
			if (!ValidateWeightedAction(
				entry,
				"AI behavior combatActions",
				catalog,
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
				catalog,
				outError))
			{
				return false;
			}
		}
	}

	return true;
}

bool GamePlayDefValidator::ValidateCharacterDefs(
	const GameDataCatalog& catalog,
	std::string& outError)
{
	const GameplayContentCatalogSnapshot* content =
		GameplayContentCatalogSnapshot::TryCurrent();
	if (content == nullptr)
	{
		outError = "Gameplay content catalog is not published.";
		return false;
	}

	for (const CharacterDef& def : catalog.Characters().GetAll())
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

		if (def.HasFeature(CharacterFeatureFlags::Combatant) &&
			!def.abilitySetKey)
		{
			outError = "Combatant character requires abilitySetKey.";
			return false;
		}

		if (def.abilitySetKey &&
			content->FindAbilitySetByKey(*def.abilitySetKey) == nullptr)
		{
			outError = "Character references unknown ability set.";
			return false;
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

			if (def.ai->aiProfileId != InvalidAIBehaviorProfileId)
			{
				const AIBehaviorProfileDef* profile =
					catalog.AIBehaviors().Find(def.ai->aiProfileId);
				if (profile == nullptr)
				{
					outError =
						"Character references missing AI behavior profile.";
					return false;
				}

				if (profile->aiType != def.ai->aiType)
				{
					outError =
						"Character AI archetype does not match AI behavior profile.";
					return false;
				}

				if (!AIFSMRegistry::IsBehaviorSupported(def.ai->aiProfileId))
				{
					outError =
						"Character references unsupported AI behavior.";
					return false;
				}
			}
			else
			{
				outError = "AIControlled character requires aiProfileKey.";
				return false;
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

bool GamePlayDefValidator::ValidateSpawnSetDefs(
	const GameDataCatalog& catalog,
	std::string& outError)
{
	for (const SpawnSetDef& def : catalog.SpawnSets().GetAll())
	{
		if (def.id == SpawnSetId::None)
		{
			outError = "SpawnSet id must not be None.";
			return false;
		}

		if (def.key.empty())
		{
			outError = "SpawnSet key must not be empty.";
			return false;
		}

		if (def.name.empty())
		{
			outError = "SpawnSet name must not be empty.";
			return false;
		}

		for (const SpawnEntryDef& entry : def.entries)
		{
			if (catalog.Characters().Find(entry.characterId) == nullptr)
			{
				outError = "SpawnSet references unknown character.";
				return false;
			}
		}
	}

	return true;
}
