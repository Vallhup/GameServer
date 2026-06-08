#include "pch.h"
#include "DefEnumString.h"

#include "AnimationId.h"
#include "CharacterDef.h"
#include "AIBehaviorDef.h"
#include "SpawnSetDef.h"
#include "TitleDef.h"
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

bool ParseDefString(std::string_view text, AIActionDistanceBucket& outValue) noexcept
{
	if (text == "Any") { outValue = AIActionDistanceBucket::Any; return true; }
	if (text == "VeryClose") { outValue = AIActionDistanceBucket::VeryClose; return true; }
	if (text == "Close") { outValue = AIActionDistanceBucket::Close; return true; }
	if (text == "Mid") { outValue = AIActionDistanceBucket::Mid; return true; }
	if (text == "Far") { outValue = AIActionDistanceBucket::Far; return true; }
	return false;
}

bool ParseDefString(std::string_view text, AIActionRole& outValue) noexcept
{
	if (text == "Basic") { outValue = AIActionRole::Basic; return true; }
	if (text == "Effect") { outValue = AIActionRole::Effect; return true; }
	return false;
}

bool ParseDefString(std::string_view text, AIMovementBehavior& outValue) noexcept
{
	if (text == "None") { outValue = AIMovementBehavior::None; return true; }
	if (text == "Hold") { outValue = AIMovementBehavior::Hold; return true; }
	if (text == "Approach") { outValue = AIMovementBehavior::Approach; return true; }
	if (text == "RunApproach") { outValue = AIMovementBehavior::RunApproach; return true; }
	if (text == "Retreat") { outValue = AIMovementBehavior::Retreat; return true; }
	if (text == "Strafe") { outValue = AIMovementBehavior::Strafe; return true; }
	if (text == "CircleLeft") { outValue = AIMovementBehavior::CircleLeft; return true; }
	if (text == "CircleRight") { outValue = AIMovementBehavior::CircleRight; return true; }
	if (text == "SearchLastKnown") { outValue = AIMovementBehavior::SearchLastKnown; return true; }
	if (text == "ReturnHome") { outValue = AIMovementBehavior::ReturnHome; return true; }
	return false;
}

bool ParseDefString(std::string_view text, AIReactionRuleEvent& outValue) noexcept
{
	if (text == "OnHitReceived") { outValue = AIReactionRuleEvent::OnHitReceived; return true; }
	if (text == "OnParried") { outValue = AIReactionRuleEvent::OnParried; return true; }
	if (text == "OnGuardBroken") { outValue = AIReactionRuleEvent::OnGuardBroken; return true; }
	if (text == "OnHpThreshold") { outValue = AIReactionRuleEvent::OnHpThreshold; return true; }
	return false;
}

bool ParseDefString(std::string_view text, AIReactionRuleOutcome& outValue) noexcept
{
	if (text == "Ignore") { outValue = AIReactionRuleOutcome::Ignore; return true; }
	if (text == "ForceRetarget") { outValue = AIReactionRuleOutcome::ForceRetarget; return true; }
	if (text == "EnterReact") { outValue = AIReactionRuleOutcome::EnterReact; return true; }
	if (text == "IssueAbility") { outValue = AIReactionRuleOutcome::IssueAbility; return true; }
	if (text == "EnterReactAndIssueAbility") { outValue = AIReactionRuleOutcome::EnterReactAndIssueAbility; return true; }
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
		{ "Knight_AttackCombo1", AnimationId::Knight_AttackCombo1 },
		{ "Knight_AttackCombo2", AnimationId::Knight_AttackCombo2 },
		{ "Knight_AttackCombo3", AnimationId::Knight_AttackCombo3 },
		{ "Knight_AttackStrong", AnimationId::Knight_AttackStrong },
		{ "Knight_AttackSpecial", AnimationId::Knight_AttackSpecial },
		{ "Knight_Dodge", AnimationId::Knight_Dodge },
		{ "Knight_Parry", AnimationId::Knight_Parry },
		{ "Knight_Stun", AnimationId::Knight_Stun },
		{ "Knight_Hit", AnimationId::Knight_Hit },
		{ "Knight_Guard", AnimationId::Knight_Guard },
		{ "Knight_Drinking", AnimationId::Knight_Drinking },
		{ "Knight_Death", AnimationId::Knight_Death },
		{ "Lancer_Idle", AnimationId::Lancer_Idle },
		{ "Lancer_Walk", AnimationId::Lancer_Walk },
		{ "Lancer_Run", AnimationId::Lancer_Run },
		{ "Lancer_AttackCombo1", AnimationId::Lancer_AttackCombo1 },
		{ "Lancer_AttackCombo2", AnimationId::Lancer_AttackCombo2 },
		{ "Lancer_AttackCombo3", AnimationId::Lancer_AttackCombo3 },
		{ "Lancer_AttackStrong", AnimationId::Lancer_AttackStrong },
		{ "Lancer_AttackSpecial", AnimationId::Lancer_AttackSpecial },
		{ "Lancer_Dodge", AnimationId::Lancer_Dodge },
		{ "Lancer_Parry", AnimationId::Lancer_Parry },
		{ "Lancer_Stun_Left", AnimationId::Lancer_StunLeft },
		{ "Lancer_Stun_Right", AnimationId::Lancer_StunRight },
		{ "Lancer_Hit", AnimationId::Lancer_Hit },
		{ "Lancer_Guard", AnimationId::Lancer_Guard },
		{ "Lancer_Drinking", AnimationId::Lancer_Drinking },
		{ "Lancer_Death", AnimationId::Lancer_Death },
		{ "Paladin_Idle", AnimationId::Paladin_Idle },
		{ "Paladin_Walk", AnimationId::Paladin_Walk },
		{ "Paladin_Run", AnimationId::Paladin_Run },
		{ "Paladin_AttackCombo1", AnimationId::Paladin_AttackCombo1 },
		{ "Paladin_AttackCombo2", AnimationId::Paladin_AttackCombo2 },
		{ "Paladin_AttackCombo3", AnimationId::Paladin_AttackCombo3 },
		{ "Paladin_AttackStrong", AnimationId::Paladin_AttackStrong },
		{ "Paladin_AttackSpecial", AnimationId::Paladin_AttackSpecial },
		{ "Paladin_Dodge", AnimationId::Paladin_Dodge },
		{ "Paladin_Parry", AnimationId::Paladin_Parry },
		{ "Paladin_Stun", AnimationId::Paladin_Stun },
		{ "Paladin_Hit", AnimationId::Paladin_Hit },
		{ "Paladin_Guard", AnimationId::Paladin_Guard },
		{ "Paladin_Drinking", AnimationId::Paladin_Drinking },
		{ "Paladin_Death", AnimationId::Paladin_Death },
		{ "FinalBoss_Idle", AnimationId::FinalBoss_Idle },
		{ "FinalBoss_Walk", AnimationId::FinalBoss_Walk },
		{ "FinalBoss_Thrust", AnimationId::FinalBoss_Thrust },
		{ "FinalBoss_Slash", AnimationId::FinalBoss_Slash },
		{ "FinalBoss_DashSlash", AnimationId::FinalBoss_DashSlash },
		{ "FinalBoss_JumpSlash", AnimationId::FinalBoss_JumpSlash },
		{ "FinalBoss_MultiSlash", AnimationId::FinalBoss_MultiSlash },
		{ "FinalBoss_BloodLance", AnimationId::FinalBoss_BloodLance },
		{ "FinalBoss_HolySandstorm", AnimationId::FinalBoss_HolySandstorm },
		{ "FinalBoss_SwordMoonlight", AnimationId::FinalBoss_SwordMoonlight },
		{ "FinalBoss_SwordStorm", AnimationId::FinalBoss_SwordStorm },
		{ "FinalBoss_Stun", AnimationId::FinalBoss_Stun },
		{ "FinalBoss_Hit", AnimationId::FinalBoss_Hit },
		{ "FinalBoss_0Percent", AnimationId::FinalBoss_0Percent },
		{ "FinalBoss_50Percent", AnimationId::FinalBoss_50Percent },
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
		{ "Tank_Idle_1", AnimationId::Tank_Idle_1 },
		{ "Tank_Idle_2", AnimationId::Tank_Idle_2 },
		{ "Tank_Idle_3", AnimationId::Tank_Idle_3 },
		{ "Tank_Idle_4", AnimationId::Tank_Idle_4 },
		{ "Tank_Idle_5", AnimationId::Tank_Idle_5 },
		{ "Tank_Melee_1", AnimationId::Tank_Melee_1 },
		{ "Tank_Melee_2", AnimationId::Tank_Melee_2 },
		{ "Tank_Melee_3", AnimationId::Tank_Melee_3 },
		{ "Tank_Melee_4", AnimationId::Tank_Melee_4 },
		{ "Tank_Melee_5", AnimationId::Tank_Melee_5 },
		{ "Tank_Melee_6", AnimationId::Tank_Melee_6 },
		{ "Tank_Melee_7", AnimationId::Tank_Melee_7 },
		{ "Tank_Melee_8", AnimationId::Tank_Melee_8 },
		{ "Tank_Walk_1", AnimationId::Tank_Walk_1 },
		{ "Tank_Walk_2", AnimationId::Tank_Walk_2 },
		{ "Tank_Walk_Back", AnimationId::Tank_WalkBack },
		{ "Tank_Walk_Left_Back", AnimationId::Tank_WalkLeftBack },
		{ "Tank_Walk_Left", AnimationId::Tank_WalkLeft },
		{ "Tank_Walk_Right_Back", AnimationId::Tank_WalkRightBack },
		{ "Tank_Walk_Right", AnimationId::Tank_WalkRight },
		{ "Tank_Turn_Left", AnimationId::Tank_TurnLeft },
		{ "Tank_Turn_Right", AnimationId::Tank_TurnRight },
		{ "Tank_Run_1", AnimationId::Tank_Running1 },
		{ "Tank_Run_2", AnimationId::Tank_Running2 },
		{ "Tank_Jump_1", AnimationId::Tank_Jump1 },
		{ "Tank_Jump_2", AnimationId::Tank_Jump2 },
		{ "Tank_Stun", AnimationId::Tank_Stun },
		{ "Tank_Death_1", AnimationId::Tank_Death1 },
		{ "Tank_Death_2", AnimationId::Tank_Death2 },
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
	if (text == "VillageMonster06") { outValue = SpawnPointIds::VillageMonster06; return true; }
	if (text == "VillageMonster01A") { outValue = SpawnPointIds::VillageMonster01A; return true; }
	if (text == "VillageMonster01B") { outValue = SpawnPointIds::VillageMonster01B; return true; }
	if (text == "VillageMonster02A") { outValue = SpawnPointIds::VillageMonster02A; return true; }
	if (text == "VillageMonster02B") { outValue = SpawnPointIds::VillageMonster02B; return true; }
	if (text == "VillageMonster05A") { outValue = SpawnPointIds::VillageMonster05A; return true; }
	if (text == "VillageMonster05B") { outValue = SpawnPointIds::VillageMonster05B; return true; }
	if (text == "VillageMonster06A") { outValue = SpawnPointIds::VillageMonster06A; return true; }
	if (text == "VillageMonster06B") { outValue = SpawnPointIds::VillageMonster06B; return true; }
	if (text == "VillageMonster07") { outValue = SpawnPointIds::VillageMonster07; return true; }
	if (text == "VillageMonster08") { outValue = SpawnPointIds::VillageMonster08; return true; }
	if (text == "VillageMonster08A") { outValue = SpawnPointIds::VillageMonster08A; return true; }
	if (text == "VillageMonster09") { outValue = SpawnPointIds::VillageMonster09; return true; }
	if (text == "VillageMonster10") { outValue = SpawnPointIds::VillageMonster10; return true; }
	if (text == "CastlePlayerStart") { outValue = SpawnPointIds::CastlePlayerStart; return true; }
	if (text == "CastleMonster01") { outValue = SpawnPointIds::CastleMonster01; return true; }
	if (text == "CastleMonster02") { outValue = SpawnPointIds::CastleMonster02; return true; }
	if (text == "CastleMonster03") { outValue = SpawnPointIds::CastleMonster03; return true; }
	if (text == "CastleMonster04") { outValue = SpawnPointIds::CastleMonster04; return true; }
	if (text == "CastleMonster05") { outValue = SpawnPointIds::CastleMonster05; return true; }
	if (text == "CastleMonster06") { outValue = SpawnPointIds::CastleMonster06; return true; }
	if (text == "CastleMonster07") { outValue = SpawnPointIds::CastleMonster07; return true; }
	if (text == "CastleMonster08") { outValue = SpawnPointIds::CastleMonster08; return true; }
	if (text == "CastleMonster09") { outValue = SpawnPointIds::CastleMonster09; return true; }
	if (text == "CastleMonster10") { outValue = SpawnPointIds::CastleMonster10; return true; }
	if (text == "CastleMonster01A") { outValue = SpawnPointIds::CastleMonster01A; return true; }
	if (text == "CastleMonster01B") { outValue = SpawnPointIds::CastleMonster01B; return true; }
	if (text == "CastleMonster03A") { outValue = SpawnPointIds::CastleMonster03A; return true; }
	if (text == "CastleMonster03B") { outValue = SpawnPointIds::CastleMonster03B; return true; }
	if (text == "CastleMonster11") { outValue = SpawnPointIds::CastleMonster11; return true; }
	if (text == "CastleMonster12") { outValue = SpawnPointIds::CastleMonster12; return true; }
	if (text == "CastleMonster07A") { outValue = SpawnPointIds::CastleMonster07A; return true; }
	if (text == "CastleMonster07B") { outValue = SpawnPointIds::CastleMonster07B; return true; }
	if (text == "CastleMonster12A") { outValue = SpawnPointIds::CastleMonster12A; return true; }
	if (text == "CastleMonster12B") { outValue = SpawnPointIds::CastleMonster12B; return true; }
	if (text == "FinalPlayerStart") { outValue = SpawnPointIds::FinalPlayerStart; return true; }
	if (text == "FinalMonster01") { outValue = SpawnPointIds::FinalMonster01; return true; }
	if (text == "PvpPlayerStartA") { outValue = SpawnPointIds::PvpPlayerStartA; return true; }
	if (text == "PvpPlayerStartB") { outValue = SpawnPointIds::PvpPlayerStartB; return true; }
	if (text == "PvpPlayerStartC") { outValue = SpawnPointIds::PvpPlayerStartC; return true; }
	return false;
}

bool ParseDefString(std::string_view text, TitleConditionType& outValue) noexcept
{
	if (text == "MonsterKill")    { outValue = TitleConditionType::MonsterKill;    return true; }
	if (text == "DeathByMonster") { outValue = TitleConditionType::DeathByMonster; return true; }
	return false;
}
