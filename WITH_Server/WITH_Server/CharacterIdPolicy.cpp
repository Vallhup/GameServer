#include "pch.h"
#include "CharacterIdPolicy.h"
#include "CharacterDef.h"

bool IsPlayableCharacterId(CharacterId id) noexcept
{
	const CharacterDef* const def = FindCharacterDef(id);
	if (def == nullptr)
	{
		return false;
	}

	return def->IsPlayable();
}

bool IsMonsterCharacterId(CharacterId id) noexcept
{
	const CharacterDef* const def = FindCharacterDef(id);
	if (def == nullptr)
	{
		return false;
	}

	return
		def->role == CharacterRole::Monster ||
		def->role == CharacterRole::Boss;
}
