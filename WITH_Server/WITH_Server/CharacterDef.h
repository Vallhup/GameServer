#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>

#include "ActionProfileIds.h"
#include "BodyCollisionTypes.h"
#include "EntityId.h"

enum class CharacterId : uint8_t;

enum class CharacterRole : uint8_t
{
	Player,
	Monster,
	Boss,
	NPC
};

enum class CharacterFeatureFlags : uint32_t
{
	None         = 0,
	Replicated   = 1 << 0,
	Combatant    = 1 << 1,
	Playable     = 1 << 2,
	AIControlled = 1 << 3,
	PortalAware  = 1 << 4,
	BuffUser     = 1 << 5,
	BossPhase    = 1 << 6
};

inline constexpr CharacterFeatureFlags operator|(
	CharacterFeatureFlags lhs,
	CharacterFeatureFlags rhs) noexcept
{
	return static_cast<CharacterFeatureFlags>(
		static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline constexpr bool HasCharacterFeature(
	CharacterFeatureFlags value,
	CharacterFeatureFlags flag) noexcept
{
	return
		(static_cast<uint32_t>(value) &
		 static_cast<uint32_t>(flag)) != 0;
}

struct CharacterProfileDef
{
	Faction faction;
};

struct CharacterStatDef
{
	uint32_t maxHp;
	uint16_t maxStamina;
	uint16_t maxPoise;
	uint32_t attackPower;
	uint16_t defense;
	float moveSpeed;
	float attackSpeed;
};

struct CharacterBodyCollisionDef
{
	float footprintRadiusXZ{ 0.5f };
	float bodyHeight{ 1.8f };
	bool blocksBodyOverlap{ true };
	bool useNavMeshConstraint{ true };
	BodyPushability pushability{ BodyPushability::Dynamic };
	float overlapYieldWeight{ 1.0f };
	float maxOverlapCorrectionPerFrameXZ{ 0.12f };
};

using AITuningId = uint16_t;

namespace AITuningIds
{
	inline constexpr AITuningId None = 0;
	inline constexpr AITuningId Imp = 1001;
	inline constexpr AITuningId DemonStriker = 1002;
	inline constexpr AITuningId DemonExecutioner = 1003;
	inline constexpr AITuningId FinalBoss = 1005;
}

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
	CharacterBodyCollisionDef bodyCollision;
	CharacterRole role{ CharacterRole::NPC };
	CharacterFeatureFlags features{ CharacterFeatureFlags::None };
	std::optional<CharacterAIDef> ai;
	std::optional<CharacterActionDefRef> action;

	bool HasFeature(CharacterFeatureFlags flag) const noexcept
	{
		return HasCharacterFeature(features, flag);
	}

	bool IsPlayable() const noexcept
	{
		return HasFeature(CharacterFeatureFlags::Playable);
	}

	bool IsAIControlled() const noexcept
	{
		return HasFeature(CharacterFeatureFlags::AIControlled);
	}

	bool IsReplicated() const noexcept
	{
		return HasFeature(CharacterFeatureFlags::Replicated);
	}
};

const CharacterDef* FindCharacterDef(CharacterId id) noexcept;
const CharacterDef& GetCharacterDef(CharacterId id);
std::span<const CharacterDef> GetCharacterDefs() noexcept;
