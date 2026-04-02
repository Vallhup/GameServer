#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>

#include "EntityId.h"

enum class CharacterId : uint8_t;

struct CharacterProfileDef
{
	Faction faction;
};

struct CharacterStatDef
{
	uint32_t maxHp;
	uint16_t maxStamina;
	uint32_t attackPower;
	uint16_t defense;
	float moveSpeed;
	float attackSpeed;
};

using AITuningId = uint16_t;

struct CharacterAIDef
{
	AIArchetype aiType;
	std::optional<AITuningId> aiTuningId;
};

struct CharacterDef
{
	CharacterId id;
	std::string name;

	CharacterProfileDef profile;
	CharacterStatDef stat;
	std::optional<CharacterAIDef> ai;
};

const CharacterDef* FindCharacterDef(CharacterId id) noexcept;
const CharacterDef& GetCharacterDef(CharacterId id);
std::span<const CharacterDef> GetCharacterDefs() noexcept;
