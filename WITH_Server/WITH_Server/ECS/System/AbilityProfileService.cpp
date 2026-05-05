#include "pch.h"
#include "AbilityProfileService.h"

#include "../../GameDataCatalog.h"
#include "../../GameplayContentCatalog.h"

#include <algorithm>

const AbilitySetDef* AbilityProfileService::FindAbilitySet(
	CharacterId characterId) noexcept
{
	const GameDataCatalog* gameDataCatalog = GameDataCatalog::TryCurrent();
	const GameplayContentCatalogSnapshot* contentCatalog = GameplayContentCatalogSnapshot::TryCurrent();
	if (gameDataCatalog == nullptr || contentCatalog == nullptr)
	{
		return nullptr;
	}

	const CharacterDef* characterDef = gameDataCatalog->Characters().Find(characterId);
	if (characterDef == nullptr || !characterDef->abilitySetKey.has_value())
	{
		return nullptr;
	}

	return contentCatalog->FindAbilitySetByKey(*characterDef->abilitySetKey);
}

const AbilityInputBindingProfileDef*
AbilityProfileService::FindInputBindingProfile(
	CharacterId characterId) noexcept
{
	const AbilitySetDef* set = FindAbilitySet(characterId);
	const GameplayContentCatalogSnapshot* catalog =
		GameplayContentCatalogSnapshot::TryCurrent();
	if (set == nullptr || catalog == nullptr ||
		!set->inputBindingProfileId.has_value())
	{
		return nullptr;
	}

	return catalog->AbilitySets().inputBindingProfiles.Find(
		*set->inputBindingProfileId);
}

const AbilityFallbackReactionProfileDef*
AbilityProfileService::FindFallbackReactionProfile(
	CharacterId characterId) noexcept
{
	const AbilitySetDef* set = FindAbilitySet(characterId);
	const GameplayContentCatalogSnapshot* catalog =
		GameplayContentCatalogSnapshot::TryCurrent();
	if (set == nullptr || catalog == nullptr ||
		!set->fallbackReactionProfileId.has_value())
	{
		return nullptr;
	}

	return catalog->AbilitySets().fallbackReactionProfiles.Find(
		*set->fallbackReactionProfileId);
}

const AbilityAnimationBindingProfileDef*
AbilityProfileService::FindAnimationBindingProfile(
	CharacterId characterId) noexcept
{
	const AbilitySetDef* set = FindAbilitySet(characterId);
	const GameplayContentCatalogSnapshot* catalog =
		GameplayContentCatalogSnapshot::TryCurrent();
	if (set == nullptr || catalog == nullptr ||
		!set->animationBindingProfileId.has_value())
	{
		return nullptr;
	}

	return catalog->AbilitySets().animationBindingProfiles.Find(
		*set->animationBindingProfileId);
}

bool AbilityProfileService::IsAbilityAvailable(
	CharacterId characterId,
	AbilityId abilityId) noexcept
{
	const AbilitySetDef* set = FindAbilitySet(characterId);
	if (set == nullptr)
	{
		return false;
	}

	return std::find_if(
		set->grants.begin(), set->grants.end(),
		[abilityId](const AbilityGrantDef& grant)
		{
			return grant.abilityId == abilityId;
		}) != set->grants.end();
}
