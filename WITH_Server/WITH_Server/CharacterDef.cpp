#include "pch.h"
#include "CharacterDef.h"
#include "EntityId.h"

#include <array>
#include <stdexcept>

namespace
{
	enum AITuning : uint16_t
	{
		ImpTuning = 1001,
	};

	const std::array<CharacterDef, 2> kCharacterDefs =
	{
		CharacterDef
		{
			.id = CharacterId::Knight,
			.name = "Knight",
			.profile = CharacterProfileDef
			{
				.faction = Faction::Player
			},
			.stat = CharacterStatDef
			{
				.maxHp = 100,
				.maxStamina = 100,
				.attackPower = 10,
				.defense = 10,
				.moveSpeed = 2.5f,
				.attackSpeed = 1.0f
			},
			.ai = std::nullopt
		},

		CharacterDef
		{
			.id = CharacterId::Imp,
			.name = "Imp",
			.profile = CharacterProfileDef
			{
				.faction = Faction::Enemy
			},
			.stat = CharacterStatDef
			{
				.maxHp = 100,
				.maxStamina = 100,
				.attackPower = 10,
				.defense = 10,
				.moveSpeed = 2.5f,
				.attackSpeed = 1.0f
			},
			.ai = CharacterAIDef
			{
				.aiType = AIArchetype::NormalMonster,
				.aiTuningId = ImpTuning
			}
		},
	};
}

const CharacterDef* FindCharacterDef(CharacterId id) noexcept
{
	for (const CharacterDef& def : kCharacterDefs)
	{
		if (def.id == id)
		{
			return &def;
		}
	}

	return nullptr;
}

const CharacterDef& GetCharacterDef(CharacterId id)
{
	const CharacterDef* const def = FindCharacterDef(id);
	if (def == nullptr)
	{
		throw std::out_of_range("CharacterDef was not found.");
	}

	return *def;
}

std::span<const CharacterDef> GetCharacterDefs() noexcept
{
	return std::span<const CharacterDef>(kCharacterDefs);
}
