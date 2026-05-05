#pragma once

#include "../../AbilitySetDef.h"
#include "../../CharacterDef.h"

class AbilityProfileService final {
public:
	static const AbilitySetDef* FindAbilitySet(CharacterId characterId) noexcept;

	static const AbilityInputBindingProfileDef* FindInputBindingProfile(
		CharacterId characterId) noexcept;

	static const AbilityFallbackReactionProfileDef* FindFallbackReactionProfile(
		CharacterId characterId) noexcept;

	static const AbilityAnimationBindingProfileDef* FindAnimationBindingProfile(
		CharacterId characterId) noexcept;

	static bool IsAbilityAvailable(
		CharacterId characterId,
		AbilityId abilityId) noexcept;
};
