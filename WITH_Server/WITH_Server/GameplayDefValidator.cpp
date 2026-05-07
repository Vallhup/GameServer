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


bool GamePlayDefValidator::ValidateAIAction(
	const AIActionDef& action,
	const char* owner,
	std::string& outError)
{
	const GameplayContentCatalogSnapshot* content =
		GameplayContentCatalogSnapshot::TryCurrent();
	if (content == nullptr)
	{
		outError = "Gameplay content catalog is not published.";
		return false;
	}

	if (action.abilityId == InvalidAbilityId ||
		content->Abilities().Find(action.abilityId) == nullptr)
	{
		outError = std::string(owner) + " references unknown ability.";
		return false;
	}

	if (action.weight == 0)
	{
		outError = std::string(owner) + " requires positive weight.";
		return false;
	}

	if (action.condition.phaseMin > action.condition.phaseMax)
	{
		outError = std::string(owner) + " has invalid phase range.";
		return false;
	}

	if (action.condition.selfHpRatioMin > action.condition.selfHpRatioMax ||
		action.condition.targetHpRatioMin > action.condition.targetHpRatioMax)
	{
		outError = std::string(owner) + " has invalid hp ratio range.";
		return false;
	}

	if (action.aiCooldownSec < 0.0f ||
		action.globalCooldownSec < 0.0f ||
		action.groupCooldownSec < 0.0f ||
		action.lockMovementSec < 0.0f)
	{
		outError = std::string(owner) + " has negative cooldown or lock.";
		return false;
	}

	return true;
}

bool GamePlayDefValidator::ValidateMovementProfile(
	const AIMovementProfileDef& profile,
	std::string& outError)
{
	if (profile.phaseMin > profile.phaseMax)
	{
		outError = "AI behavior movementProfiles has invalid phase range.";
		return false;
	}

	if (profile.strafeMinSec < 0.0f ||
		profile.strafeMaxSec < 0.0f ||
		profile.strafeMinSec > profile.strafeMaxSec)
	{
		outError = "AI behavior movementProfiles has invalid strafe range.";
		return false;
	}

	return true;
}

bool GamePlayDefValidator::ValidateReactionRule(
	const AIReactionRuleDef& rule,
	std::string& outError)
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

	if (rule.reactAbilityId != InvalidAbilityId)
	{
		const GameplayContentCatalogSnapshot* content =
			GameplayContentCatalogSnapshot::TryCurrent();
		if (content == nullptr ||
			content->Abilities().Find(rule.reactAbilityId) == nullptr)
		{
			outError = "AI behavior reactionRules references unknown ability.";
			return false;
		}
	}

	return true;
}

bool GamePlayDefValidator::ValidatePhaseTransition(
	const AIPhaseTransitionDef& transition,
	std::string& outError)
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

	if (transition.transitionAbilityId != InvalidAbilityId)
	{
		const GameplayContentCatalogSnapshot* content =
			GameplayContentCatalogSnapshot::TryCurrent();
		if (content == nullptr ||
			content->Abilities().Find(transition.transitionAbilityId) == nullptr)
		{
			outError = "AI behavior phaseTransitions references unknown ability.";
			return false;
		}
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

		for (const AIActionDef& action : profile.combatActionDefs)
		{
			if (!ValidateAIAction(
					action,
					"AI behavior combatActions",
					outError))
			{
				return false;
			}
		}

		for (const AIActionDef& action : profile.idleActionDefs)
		{
			if (!ValidateAIAction(
					action,
					"AI behavior idleActions",
					outError))
			{
				return false;
			}
		}

		for (const AIMovementProfileDef& movement : profile.movementProfiles)
		{
			if (!ValidateMovementProfile(movement, outError))
				return false;
		}

		for (const AIReactionRuleDef& rule : profile.reactionRules)
		{
			if (!ValidateReactionRule(rule, outError))
				return false;
		}

		for (const AIPhaseTransitionDef& transition : profile.phaseTransitions)
		{
			if (!ValidatePhaseTransition(transition, outError))
				return false;
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
