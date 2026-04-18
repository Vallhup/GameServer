#include "pch.h"
#include "CharacterDef.h"
#include "EntityId.h"

#include <array>
#include <stdexcept>

namespace
{
	using CFF = CharacterFeatureFlags;

	const std::array<CharacterDef, 5> kCharacterDefs =
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
				.maxPoise = 100,
				.attackPower = 10,
				.defense = 10,
				.moveSpeed = 3.0f,
				.attackSpeed = 1.0f
			},
			.bodyCollision = CharacterBodyCollisionDef
			{
				.footprintRadiusXZ = 0.42f,
				.bodyHeight = 1.8f,
				.blocksBodyOverlap = true,
				.useNavMeshConstraint = true,
				.pushability = BodyPushability::Dynamic,
				.overlapYieldWeight = 0.35f,
				.maxOverlapCorrectionPerFrameXZ = 0.10f
			},
			.role = CharacterRole::Player,
			.features =
				CFF::Replicated |
				CFF::Combatant |
				CFF::Playable |
				CFF::PortalAware |
				CFF::BuffUser,
			.ai = std::nullopt,
			.action = CharacterActionDefRef
			{
				.actionProfileId = CharacterActionProfileIds::Knight
			}
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
				.maxPoise = 100,
				.attackPower = 10,
				.defense = 10,
				.moveSpeed = 3.0f,
				.attackSpeed = 1.0f
			},
			.bodyCollision = CharacterBodyCollisionDef
			{
				.footprintRadiusXZ = 0.46f,
				.bodyHeight = 1.6f,
				.blocksBodyOverlap = true,
				.useNavMeshConstraint = true,
				.pushability = BodyPushability::Dynamic,
				.overlapYieldWeight = 0.85f,
				.maxOverlapCorrectionPerFrameXZ = 0.16f
			},
			.role = CharacterRole::Monster,
			.features =
				CFF::Replicated |
				CFF::Combatant |
				CFF::AIControlled,
			.ai = CharacterAIDef
			{
				.aiType = AIArchetype::NormalMonster,
				.aiTuningId = AITuningIds::Imp
			},
			.action = CharacterActionDefRef
			{
				.actionProfileId = CharacterActionProfileIds::Imp
			}
		},
		CharacterDef
		{
			.id = CharacterId::DemonStriker,
			.name = "DemonStriker",
			.profile = CharacterProfileDef
			{
				.faction = Faction::Enemy
			},
			.stat = CharacterStatDef
			{
				.maxHp = 100,
				.maxStamina = 100,
				.maxPoise = 100,
				.attackPower = 10,
				.defense = 10,
				.moveSpeed = 3.0f,
				.attackSpeed = 1.0f
			},
			.bodyCollision = CharacterBodyCollisionDef
			{
				.footprintRadiusXZ = 0.46f,
				.bodyHeight = 1.6f,
				.blocksBodyOverlap = true,
				.useNavMeshConstraint = true,
				.pushability = BodyPushability::Dynamic,
				.overlapYieldWeight = 0.85f,
				.maxOverlapCorrectionPerFrameXZ = 0.16f
			},
			.role = CharacterRole::Monster,
			.features =
				CFF::Replicated |
				CFF::Combatant |
				CFF::AIControlled,
			.ai = CharacterAIDef
			{
				.aiType = AIArchetype::NormalMonster,
				.aiTuningId = AITuningIds::DemonStriker
			},
			.action = CharacterActionDefRef
			{
				.actionProfileId = CharacterActionProfileIds::DemonStriker
			}
		},
		CharacterDef
		{
			.id = CharacterId::DemonExecutioner,
			.name = "DemonExecutioner",
			.profile = CharacterProfileDef
			{
				.faction = Faction::Enemy
			},
			.stat = CharacterStatDef
			{
				.maxHp = 100,
				.maxStamina = 100,
				.maxPoise = 100,
				.attackPower = 10,
				.defense = 10,
				.moveSpeed = 3.0f,
				.attackSpeed = 1.0f
			},
			.bodyCollision = CharacterBodyCollisionDef
			{
				.footprintRadiusXZ = 0.46f,
				.bodyHeight = 1.6f,
				.blocksBodyOverlap = true,
				.useNavMeshConstraint = true,
				.pushability = BodyPushability::Dynamic,
				.overlapYieldWeight = 0.85f,
				.maxOverlapCorrectionPerFrameXZ = 0.16f
			},
			.role = CharacterRole::Monster,
			.features =
				CFF::Replicated |
				CFF::Combatant |
				CFF::AIControlled,
			.ai = CharacterAIDef
			{
				.aiType = AIArchetype::NormalMonster,
				.aiTuningId = AITuningIds::DemonExecutioner
			},
			.action = CharacterActionDefRef
			{
				.actionProfileId = CharacterActionProfileIds::DemonExecutioner
			}
		},
		CharacterDef
		{
			.id = CharacterId::FinalBoss,
			.name = "FinalBoss",
			.profile = CharacterProfileDef
			{
				.faction = Faction::Enemy
			},
			.stat = CharacterStatDef
			{
				.maxHp = 100,
				.maxStamina = 100,
				.maxPoise = 100,
				.attackPower = 10,
				.defense = 10,
				.moveSpeed = 3.0f,
				.attackSpeed = 1.0f
			},
			.bodyCollision = CharacterBodyCollisionDef
			{
				.footprintRadiusXZ = 0.65f,
				.bodyHeight = 2.4f,
				.blocksBodyOverlap = true,
				.useNavMeshConstraint = true,
				.pushability = BodyPushability::Dynamic,
				.overlapYieldWeight = 0.20f,
				.maxOverlapCorrectionPerFrameXZ = 0.08f
			},
			.role = CharacterRole::Boss,
			.features =
				CFF::Replicated |
				CFF::Combatant |
				CFF::AIControlled |
				CFF::BossPhase,
			.ai = CharacterAIDef
			{
				.aiType = AIArchetype::FinalBossMonster,
				.aiTuningId = AITuningIds::FinalBoss
			},
			.action = CharacterActionDefRef
			{
				.actionProfileId = CharacterActionProfileIds::FinalBoss
			}
		}
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
