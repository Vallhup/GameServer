#include "pch.h"
#include "DefEnumString.h"

#include "AnimationId.h"
#include "CharacterDef.h"
#include "SpawnSetDef.h"
#include "ECS/Components/GameplayAbilityComponents.h"

#include <unordered_map>

bool ParseDefString(std::string_view text, AIArchetype& outValue) noexcept
{
	if (text == "None") { outValue = AIArchetype::None; return true; }
	if (text == "Humanoid") { outValue = AIArchetype::Humanoid; return true; }
	if (text == "NormalMonster") { outValue = AIArchetype::NormalMonster; return true; }
	if (text == "FirstBossMonster") { outValue = AIArchetype::FirstBossMonster; return true; }
	if (text == "MidBossMonster") { outValue = AIArchetype::MidBossMonster; return true; }
	if (text == "FinalBossMonster") { outValue = AIArchetype::FinalBossMonster; return true; }
	return false;
}

bool ParseDefString(std::string_view text, BodyPushability& outValue) noexcept
{
	if (text == "None") { outValue = BodyPushability::None; return true; }
	if (text == "Kinematic") { outValue = BodyPushability::Kinematic; return true; }
	if (text == "Dynamic") { outValue = BodyPushability::Dynamic; return true; }
	return false;
}

bool ParseAnimationIdString(std::string_view text, AnimationId& outValue) noexcept
{
	static const std::unordered_map<std::string_view, AnimationId> kValues =
	{
		{ "None", AnimationId::None },
		{ "Knight_Idle", AnimationId::Knight_Idle },
		{ "Knight_Walk", AnimationId::Knight_Walk },
		{ "Knight_Run", AnimationId::Knight_Run },
		{ "Knight_LightAttack1", AnimationId::Knight_LightAttack1 },
		{ "Knight_Dodge", AnimationId::Knight_Dodge },
		{ "Knight_Parry", AnimationId::Knight_Parry },
		{ "Knight_Stun", AnimationId::Knight_Stun },
		{ "Knight_Hit", AnimationId::Knight_Hit },
		{ "Knight_Guard", AnimationId::Knight_Guard },
		{ "Knight_Drinking", AnimationId::Knight_Drinking },
		{ "Knight_Death", AnimationId::Knight_Death },
		{ "FinalBoss_Idle", AnimationId::FinalBoss_Idle },
		{ "FinalBoss_Walk", AnimationId::FinalBoss_Walk },
		{ "FinalBoss_Thrust", AnimationId::FinalBoss_Thrust },
		{ "FinalBoss_Slash", AnimationId::FinalBoss_Slash },
		{ "FinalBoss_DashSlash", AnimationId::FinalBoss_DashSlash },
		{ "FinalBoss_JumpSlash", AnimationId::FinalBoss_JumpSlash },
		{ "FinalBoss_MultiSlash", AnimationId::FinalBoss_MultiSlash },
		{ "FinalBoss_Stun", AnimationId::FinalBoss_Stun },
		{ "FinalBoss_Hit", AnimationId::FinalBoss_Hit },
		{ "FinalBoss_Death", AnimationId::FinalBoss_Death },
		{ "Imp_Idle_1", AnimationId::Imp_Idle_1 },
		{ "Imp_Melee_1", AnimationId::Imp_Melee_1 },
		{ "Imp_Melee_2", AnimationId::Imp_Melee_2 },
		{ "Imp_Melee_3", AnimationId::Imp_Melee_3 },
		{ "Imp_Melee_4", AnimationId::Imp_Melee_4 },
		{ "Imp_Melee_5", AnimationId::Imp_Melee_5 },
		{ "Imp_Walk_Forward", AnimationId::Imp_Walk_Forward },
		{ "Imp_Jump_1", AnimationId::Imp_Jump_1 },
		{ "Imp_Stun", AnimationId::Imp_Stun },
		{ "Imp_React_Front", AnimationId::Imp_React_Front },
		{ "Imp_Death_1", AnimationId::Imp_Death_1 },
		{ "DemonStriker_Idle_1", AnimationId::DemonStriker_Idle_1 },
		{ "DemonStriker_Melee_1", AnimationId::DemonStriker_Melee_1 },
		{ "DemonStriker_Melee_2", AnimationId::DemonStriker_Melee_2 },
		{ "DemonStriker_Melee_3", AnimationId::DemonStriker_Melee_3 },
		{ "DemonStriker_Melee_4", AnimationId::DemonStriker_Melee_4 },
		{ "DemonStriker_Gun_Shoot_1", AnimationId::DemonStriker_Gun_Shoot_1 },
		{ "DemonStriker_Gun_Shoot_2", AnimationId::DemonStriker_Gun_Shoot_2 },
		{ "DemonStriker_Gun_Shoot_3", AnimationId::DemonStriker_Gun_Shoot_3 },
		{ "DemonStriker_Gun_Shoot_4", AnimationId::DemonStriker_Gun_Shoot_4 },
		{ "DemonStriker_Walking_1", AnimationId::DemonStriker_Walking_1 },
		{ "DemonStriker_Running_1", AnimationId::DemonStriker_Running_1 },
		{ "DemonStriker_Jump_1", AnimationId::DemonStriker_Jump_1 },
		{ "DemonStriker_Jump_2", AnimationId::DemonStriker_Jump_2 },
		{ "DemonStriker_Stun", AnimationId::DemonStriker_Stun },
		{ "DemonStriker_React_1_Block", AnimationId::DemonStriker_React_1_Block },
		{ "DemonStriker_Death_1", AnimationId::DemonStriker_Death_1 },
		{ "DemonExecutioner_Idle_2", AnimationId::DemonExecutioner_Idle_2 },
		{ "DemonExecutioner_Melee_1", AnimationId::DemonExecutioner_Melee_1 },
		{ "DemonExecutioner_Melee_2", AnimationId::DemonExecutioner_Melee_2 },
		{ "DemonExecutioner_Melee_3", AnimationId::DemonExecutioner_Melee_3 },
		{ "DemonExecutioner_Melee_4", AnimationId::DemonExecutioner_Melee_4 },
		{ "DemonExecutioner_Melee_5", AnimationId::DemonExecutioner_Melee_5 },
		{ "DemonExecutioner_Melee_6", AnimationId::DemonExecutioner_Melee_6 },
		{ "DemonExecutioner_Walk_Forward", AnimationId::DemonExecutioner_Walk_Forward },
		{ "DemonExecutioner_Run", AnimationId::DemonExecutioner_Run },
		{ "DemonExecutioner_Jump_1", AnimationId::DemonExecutioner_Jump_1 },
		{ "DemonExecutioner_Jump_2", AnimationId::DemonExecutioner_Jump_2 },
		{ "DemonExecutioner_Stun", AnimationId::DemonExecutioner_Stun },
		{ "DemonExecutioner_GetHit_1", AnimationId::DemonExecutioner_GetHit_1 },
		{ "DemonExecutioner_Death", AnimationId::DemonExecutioner_Death },
		{ "BigDemonWarrior_Idle_1", AnimationId::BigDemonWarrior_Idle_1 },
		{ "BigDemonWarrior_Idle_2", AnimationId::BigDemonWarrior_Idle_2 },
		{ "BigDemonWarrior_Idle_3", AnimationId::BigDemonWarrior_Idle_3 },
		{ "BigDemonWarrior_Idle_4", AnimationId::BigDemonWarrior_Idle_4 },
		{ "BigDemonWarrior_BattleCry", AnimationId::BigDemonWarrior_BattleCry },
		{ "BigDemonWarrior_Roaring", AnimationId::BigDemonWarrior_Roaring },
		{ "BigDemonWarrior_Melee_1", AnimationId::BigDemonWarrior_Melee_1 },
		{ "BigDemonWarrior_Melee_2", AnimationId::BigDemonWarrior_Melee_2 },
		{ "BigDemonWarrior_Melee_3", AnimationId::BigDemonWarrior_Melee_3 },
		{ "BigDemonWarrior_Melee_4", AnimationId::BigDemonWarrior_Melee_4 },
		{ "BigDemonWarrior_Melee_5", AnimationId::BigDemonWarrior_Melee_5 },
		{ "BigDemonWarrior_Melee_6", AnimationId::BigDemonWarrior_Melee_6 },
		{ "BigDemonWarrior_Melee_7", AnimationId::BigDemonWarrior_Melee_7 },
		{ "BigDemonWarrior_Melee_8", AnimationId::BigDemonWarrior_Melee_8 },
		{ "BigDemonWarrior_Walk_Back", AnimationId::BigDemonWarrior_WalkBack },
		{ "BigDemonWarrior_Walk_Forward", AnimationId::BigDemonWarrior_WalkForward },
		{ "BigDemonWarrior_Walk_Left", AnimationId::BigDemonWarrior_WalkLeft },
		{ "BigDemonWarrior_Walk_Right", AnimationId::BigDemonWarrior_WalkRight },
		{ "BigDemonWarrior_Run", AnimationId::BigDemonWarrior_RunForward },
		{ "BigDemonWarrior_Turn_Left", AnimationId::BigDemonWarrior_TurnLeft },
		{ "BigDemonWarrior_Turn_Right", AnimationId::BigDemonWarrior_TurnRight },
		{ "BigDemonWarrior_Jump", AnimationId::BigDemonWarrior_Jump },
		{ "BigDemonWarrior_Stun", AnimationId::BigDemonWarrior_Stun },
		{ "BigDemonWarrior_React_Left", AnimationId::BigDemonWarrior_ReactFromLeft },
		{ "BigDemonWarrior_React_Right", AnimationId::BigDemonWarrior_ReactFromRight },
		{ "BigDemonWarrior_React_Gut", AnimationId::BigDemonWarrior_ReactGut },
		{ "BigDemonWarrior_Death", AnimationId::BigDemonWarrior_Death },
	};

	const auto it = kValues.find(text);
	if (it == kValues.end())
		return false;

	outValue = it->second;
	return true;
}

bool ParseDefString(std::string_view text, CharacterFeatureFlags& outValue) noexcept
{
	if (text == "None") { outValue = CharacterFeatureFlags::None; return true; }
	if (text == "Replicated") { outValue = CharacterFeatureFlags::Replicated; return true; }
	if (text == "Combatant") { outValue = CharacterFeatureFlags::Combatant; return true; }
	if (text == "Playable") { outValue = CharacterFeatureFlags::Playable; return true; }
	if (text == "AIControlled") { outValue = CharacterFeatureFlags::AIControlled; return true; }
	if (text == "PortalAware") { outValue = CharacterFeatureFlags::PortalAware; return true; }
	if (text == "EffectUser") { outValue = CharacterFeatureFlags::EffectUser; return true; }
	if (text == "BossPhase") { outValue = CharacterFeatureFlags::BossPhase; return true; }
	return false;
}

bool ParseDefString(std::string_view text, CharacterId& outValue) noexcept
{
	if (text == "None") { outValue = CharacterId::None; return true; }
	if (text == "Knight") { outValue = CharacterId::Knight; return true; }
	if (text == "Lancer") { outValue = CharacterId::Lancer; return true; }
	if (text == "Paladin") { outValue = CharacterId::Paladin; return true; }
	if (text == "Imp") { outValue = CharacterId::Imp; return true; }
	if (text == "DemonStriker") { outValue = CharacterId::DemonStriker; return true; }
	if (text == "DemonExecutioner") { outValue = CharacterId::DemonExecutioner; return true; }
	if (text == "BigDemonWarrior") { outValue = CharacterId::BigDemonWarrior; return true; }
	if (text == "Tank") { outValue = CharacterId::Tank; return true; }
	if (text == "FinalBoss") { outValue = CharacterId::FinalBoss; return true; }
	return false;
}

bool ParseDefString(std::string_view text, CharacterRole& outValue) noexcept
{
	if (text == "Player") { outValue = CharacterRole::Player; return true; }
	if (text == "Monster") { outValue = CharacterRole::Monster; return true; }
	if (text == "Boss") { outValue = CharacterRole::Boss; return true; }
	if (text == "NPC") { outValue = CharacterRole::NPC; return true; }
	return false;
}

bool ParseDefString(std::string_view text, Faction& outValue) noexcept
{
	if (text == "None") { outValue = Faction::None; return true; }
	if (text == "Neutral") { outValue = Faction::Neutral; return true; }
	if (text == "Player") { outValue = Faction::Player; return true; }
	if (text == "Enemy") { outValue = Faction::Enemy; return true; }
	return false;
}

bool ParseDefString(std::string_view text, LocomotionMode& outValue) noexcept
{
	if (text == "Idle") { outValue = LocomotionMode::Idle; return true; }
	if (text == "Walk") { outValue = LocomotionMode::Walk; return true; }
	if (text == "Run") { outValue = LocomotionMode::Run; return true; }
	if (text == "Turn") { outValue = LocomotionMode::Turn; return true; }
	if (text == "WalkBack") { outValue = LocomotionMode::WalkBack; return true; }
	if (text == "WalkLeft") { outValue = LocomotionMode::WalkLeft; return true; }
	if (text == "WalkRight") { outValue = LocomotionMode::WalkRight; return true; }
	if (text == "TurnLeft") { outValue = LocomotionMode::TurnLeft; return true; }
	if (text == "TurnRight") { outValue = LocomotionMode::TurnRight; return true; }
	return false;
}

bool ParseDefString(std::string_view text, RespawnPolicy& outValue) noexcept
{
	if (text == "None") { outValue = RespawnPolicy::None; return true; }
	if (text == "Delay") { outValue = RespawnPolicy::Delay; return true; }
	return false;
}

bool ParseDefString(std::string_view text, SpawnConditionType& outValue) noexcept
{
	if (text == "Always") { outValue = SpawnConditionType::Always; return true; }
	if (text == "OnWorldStart") { outValue = SpawnConditionType::OnWorldStart; return true; }
	if (text == "OnRespawn") { outValue = SpawnConditionType::OnRespawn; return true; }
	return false;
}

bool ParseDefString(std::string_view text, SpawnSetId& outValue) noexcept
{
	if (text == "None") { outValue = SpawnSetId::None; return true; }
	if (text == "PlazaDefault") { outValue = SpawnSetId::PlazaDefault; return true; }
	if (text == "VillageDefault") { outValue = SpawnSetId::VillageDefault; return true; }
	if (text == "CastleDefault") { outValue = SpawnSetId::CastleDefault; return true; }
	if (text == "FinalDefault") { outValue = SpawnSetId::FinalDefault; return true; }
	if (text == "PvpDefault") { outValue = SpawnSetId::PvpDefault; return true; }
	return false;
}

bool ParseSpawnPointIdString(std::string_view text, SpawnPointId& outValue) noexcept
{
	if (text == "None") { outValue = SpawnPointIds::None; return true; }
	if (text == "PlazaPlayerStart") { outValue = SpawnPointIds::PlazaPlayerStart; return true; }
	if (text == "PlazaMonster01") { outValue = SpawnPointIds::PlazaMonster01; return true; }
	if (text == "VillagePlayerStart") { outValue = SpawnPointIds::VillagePlayerStart; return true; }
	if (text == "VillageMonster01") { outValue = SpawnPointIds::VillageMonster01; return true; }
	if (text == "VillageMonster02") { outValue = SpawnPointIds::VillageMonster02; return true; }
	if (text == "VillageMonster03") { outValue = SpawnPointIds::VillageMonster03; return true; }
	if (text == "VillageMonster04") { outValue = SpawnPointIds::VillageMonster04; return true; }
	if (text == "VillageMonster05") { outValue = SpawnPointIds::VillageMonster05; return true; }
	if (text == "VillageBossMonster01") { outValue = SpawnPointIds::VillageBossMonster01; return true; }
	if (text == "CastlePlayerStart") { outValue = SpawnPointIds::CastlePlayerStart; return true; }
	if (text == "CastleMonster01") { outValue = SpawnPointIds::CastleMonster01; return true; }
	if (text == "CastleMonster02") { outValue = SpawnPointIds::CastleMonster02; return true; }
	if (text == "CastleMonster03") { outValue = SpawnPointIds::CastleMonster03; return true; }
	if (text == "CastleMonster04") { outValue = SpawnPointIds::CastleMonster04; return true; }
	if (text == "CastleMonster05") { outValue = SpawnPointIds::CastleMonster05; return true; }
	if (text == "CastleMonster06") { outValue = SpawnPointIds::CastleMonster06; return true; }
	if (text == "FinalPlayerStart") { outValue = SpawnPointIds::FinalPlayerStart; return true; }
	if (text == "FinalMonster01") { outValue = SpawnPointIds::FinalMonster01; return true; }
	if (text == "PvpPlayerStartA") { outValue = SpawnPointIds::PvpPlayerStartA; return true; }
	if (text == "PvpPlayerStartB") { outValue = SpawnPointIds::PvpPlayerStartB; return true; }
	return false;
}
