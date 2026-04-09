#pragma once

#include "../../ActionDef.h"
#include "../../CharacterDef.h"

class ActionProfileService final
{
public:
	const CharacterActionProfileDef* FindCharacterProfile(
		CharacterId characterId) const noexcept;

	const ActionInputBindingProfileDef* FindInputBindingProfile(
		CharacterId characterId) const noexcept;

	const ActionFallbackReactionProfileDef* FindFallbackReactionProfile(
		CharacterId characterId) const noexcept;

	const AnimationBindingProfileDef* FindAnimationBindingProfile(
		CharacterId characterId) const noexcept;

	bool IsActionAvailable(
		CharacterId characterId,
		ActionId actionId) const noexcept;
};
