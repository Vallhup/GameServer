#include "pch.h"
#include "CharacterIdPolicy.h"
#include "CharacterDef.h"
#include "GameDataCatalog.h"

bool IsPlayableCharacterId(CharacterId id) noexcept
{
	const GameDataCatalog* catalog = GameDataCatalog::TryCurrent();
	if (catalog == nullptr)
	{
		return false;
	}

	const CharacterDef* const def =
		catalog->Characters().Find(id);
	if (def == nullptr)
	{
		return false;
	}

	return def->IsPlayable();
}

bool IsMonsterCharacterId(CharacterId id) noexcept
{
	const GameDataCatalog* catalog = GameDataCatalog::TryCurrent();
	if (catalog == nullptr)
	{
		return false;
	}

	const CharacterDef* const def =
		catalog->Characters().Find(id);
	if (def == nullptr)
	{
		return false;
	}

	return
		def->role == CharacterRole::Monster ||
		def->role == CharacterRole::Boss;
}
