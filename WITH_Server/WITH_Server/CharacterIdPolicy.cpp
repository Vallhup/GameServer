#include "pch.h"
#include "CharacterIdPolicy.h"

bool IsPlayableCharacterId(CharacterId id) noexcept
{
	switch (id) {
	case CharacterId::Knight:
	case CharacterId::Lancer:
	case CharacterId::Vanguard:
		return true;

	default:
		return false;
	}
}

bool IsMonsterCharacterId(CharacterId id) noexcept
{
	switch (id) {
	case CharacterId::Imp:
	case CharacterId::DemonStriker:
	case CharacterId::DemonExecutioner:
	case CharacterId::BigDemonWarrior:
	case CharacterId::Tank:
	case CharacterId::FinalBoss:
		return true;

	default:
		return false;
	}
}
