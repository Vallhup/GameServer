#pragma once

#include "EntityId.h"
#include <string>
#include <optional>

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