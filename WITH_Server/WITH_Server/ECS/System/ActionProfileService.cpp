#include "pch.h"
#include "ActionProfileService.h"

#include <algorithm>

const CharacterActionProfileDef* ActionProfileService::FindCharacterProfile(
	CharacterId characterId) const noexcept
{
	const CharacterDef* characterDef = FindCharacterDef(characterId);
	if (characterDef == nullptr || !characterDef->action.has_value())
	{
		return nullptr;
	}

	return FindCharacterActionProfileDef(characterDef->action->actionProfileId);
}

const ActionInputBindingProfileDef* ActionProfileService::FindInputBindingProfile(
	CharacterId characterId) const noexcept
{
	const CharacterActionProfileDef* profile = FindCharacterProfile(characterId);
	return (profile != nullptr)
		? FindActionInputBindingProfileDef(profile->inputBindingProfileId)
		: nullptr;
}

const ActionFallbackReactionProfileDef* ActionProfileService::FindFallbackReactionProfile(
	CharacterId characterId) const noexcept
{
	const CharacterActionProfileDef* profile = FindCharacterProfile(characterId);
	return (profile != nullptr)
		? FindActionFallbackReactionProfileDef(profile->fallbackReactionProfileId)
		: nullptr;
}

const AnimationBindingProfileDef* ActionProfileService::FindAnimationBindingProfile(
	CharacterId characterId) const noexcept
{
	const CharacterActionProfileDef* profile = FindCharacterProfile(characterId);
	return (profile != nullptr)
		? FindAnimationBindingProfileDef(profile->animationBindingProfileId)
		: nullptr;
}

bool ActionProfileService::IsActionAvailable(
	CharacterId characterId,
	ActionId actionId) const noexcept
{
	const CharacterActionProfileDef* profile = FindCharacterProfile(characterId);
	if (profile == nullptr)
	{
		return false;
	}

	return std::find(
		profile->availableActions.begin(),
		profile->availableActions.end(),
		actionId) != profile->availableActions.end();
}
