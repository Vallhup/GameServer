#include "pch.h"
#include "AnimationJsonLoader.h"
#include "json.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

namespace
{
	bool ReadVector3(const json& node, XMFLOAT3& outValue)
	{
		if (!node.is_array() || node.size() != 3)
		{
			return false;
		}

		outValue.x = node[0].get<float>();
		outValue.y = node[1].get<float>();
		outValue.z = node[2].get<float>();
		return true;
	}
}

AnimationJsonLoader::LoadResult AnimationJsonLoader::LoadFile(
	const std::filesystem::path& path,
	AnimationClipDef& outDef) const
{
	LoadResult result{};

	std::ifstream input(path);
	if (!input.is_open())
	{
		result.error = "Failed to open animation json file: " + path.string();
		return result;
	}

	std::stringstream buffer;
	buffer << input.rdbuf();

	if (!ParseDocument(buffer.str(), outDef, result.error))
	{
		return result;
	}

	result.succeeded = true;
	result.loadedCount = 1;
	return result;
}

AnimationJsonLoader::LoadResult AnimationJsonLoader::LoadDirectory(
	const std::filesystem::path& directory,
	std::vector<AnimationClipDef>& outDefs) const
{
	LoadResult result{};
	outDefs.clear();

	if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
	{
		result.error = "Animation json directory was not found: " + directory.string();
		return result;
	}

	std::vector<std::filesystem::path> files;
	for (const auto& entry : std::filesystem::directory_iterator(directory))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".json")
		{
			files.push_back(entry.path());
		}
	}

	std::sort(files.begin(), files.end());

	for (const auto& file : files)
	{
		AnimationClipDef def;
		LoadResult fileResult = LoadFile(file, def);
		if (!fileResult.succeeded)
		{
			result.error = fileResult.error;
			outDefs.clear();
			return result;
		}

		outDefs.push_back(std::move(def));
	}

	result.succeeded = true;
	result.loadedCount = outDefs.size();
	return result;
}

bool AnimationJsonLoader::ParseDocument(
	const std::string& jsonText,
	AnimationClipDef& outDef,
	std::string& outError) const
{
	json root;
	try
	{
		root = json::parse(jsonText);
	}
	catch (const std::exception& ex)
	{
		outError = "Failed to parse animation json: " + std::string(ex.what());
		return false;
	}

	try
	{
		outDef.version = root.at("version").get<uint16_t>();
		outDef.clipId = root.at("clipId").get<std::string>();
		outDef.skeleton = root.at("skeleton").get<std::string>();
		outDef.fps = root.at("fps").get<float>();
		outDef.numFrames = root.at("numFrames").get<uint16_t>();
		outDef.durationSec = root.at("durationSec").get<float>();
		outDef.units = root.at("units").get<std::string>();
		outDef.loop = root.at("loop").get<bool>();
		outDef.source = root.at("source").get<std::string>();
	}
	catch (const std::exception& ex)
	{
		outError = "Animation json metadata is invalid: " + std::string(ex.what());
		return false;
	}

	if (!ResolveAnimationId(outDef.clipId, outDef.id, outError))
	{
		return false;
	}

	try
	{
		outDef.capsuleDefs.clear();
		for (const auto& capsuleNode : root.at("capsules"))
		{
			AnimationCapsuleDef capsuleDef;
			capsuleDef.boneIndex = capsuleNode.at("bone").get<uint16_t>();
			capsuleDef.radius = capsuleNode.at("radius").get<float>();

			for (const auto& roleNode : capsuleNode.at("roles"))
			{
				CapsuleRole role = CapsuleRole::None;
				if (!ParseCapsuleRole(roleNode.get<std::string>(), role, outError))
				{
					return false;
				}

				capsuleDef.roles.push_back(role);
			}

			outDef.capsuleDefs.push_back(std::move(capsuleDef));
		}

		outDef.frames.clear();
		for (const auto& frameNode : root.at("frames"))
		{
			AnimationClipFrame frame;
			for (const auto& capsuleNode : frameNode)
			{
				Capsule capsule;
				if (!ReadVector3(capsuleNode.at("p0"), capsule.p0) ||
					!ReadVector3(capsuleNode.at("p1"), capsule.p1))
				{
					outError = "Animation capsule frame p0/p1 must be float[3].";
					return false;
				}

				frame.capsules.push_back(capsule);
			}

			outDef.frames.push_back(std::move(frame));
		}
	}
	catch (const std::exception& ex)
	{
		outError = "Animation json frame data is invalid: " + std::string(ex.what());
		return false;
	}

	return ValidateParsedDef(outDef, outError);
}

bool AnimationJsonLoader::ResolveAnimationId(
	std::string_view clipId,
	AnimationId& outId,
	std::string& outError) const
{
	static const std::unordered_map<std::string_view, AnimationId> kClipIdToAnimationId =
	{
		{ "Knight_idle", AnimationId::Knight_Idle },
		{ "Knight_walk", AnimationId::Knight_Walk },
		{ "Knight_run", AnimationId::Knight_Run },
		{ "Knight_attack_combo01", AnimationId::Knight_AttackCombo1 },
		{ "Knight_attack_combo02", AnimationId::Knight_AttackCombo2 },
		{ "Knight_attack_combo03", AnimationId::Knight_AttackCombo3 },
		{ "Knight_attack_strong", AnimationId::Knight_AttackStrong },
		{ "Knight_specialattack", AnimationId::Knight_AttackSpecial },
		{ "Knight_dodge", AnimationId::Knight_Dodge },
		{ "Knight_parry", AnimationId::Knight_Parry },
		{ "Knight_stun", AnimationId::Knight_Stun },
		{ "Knight_hit", AnimationId::Knight_Hit },
		{ "Knight_guard", AnimationId::Knight_Guard },
		{ "Knight_drinking", AnimationId::Knight_Drinking },
		{ "Knight_death", AnimationId::Knight_Death },
		{ "Lancer_idle", AnimationId::Lancer_Idle },
		{ "Lancer_walk", AnimationId::Lancer_Walk },
		{ "Lancer_run", AnimationId::Lancer_Run },
		{ "Lancer_lightattack1", AnimationId::Lancer_AttackCombo1 },
		{ "Lancer_lightattack2", AnimationId::Lancer_AttackCombo2 },
		{ "Lancer_lightattack3", AnimationId::Lancer_AttackCombo3 },
		{ "Lancer_heavyattack", AnimationId::Lancer_AttackStrong },
		{ "Lancer_specialattack", AnimationId::Lancer_AttackSpecial },
		{ "Lancer_dodge", AnimationId::Lancer_Dodge },
		{ "Lancer_parry", AnimationId::Lancer_Parry },
		{ "Lancer_stun_left", AnimationId::Lancer_StunLeft },
		{ "Lancer_stun_right", AnimationId::Lancer_StunRight },
		{ "Lancer_hit", AnimationId::Lancer_Hit },
		{ "Lancer_guard", AnimationId::Lancer_Guard },
		{ "Lancer_drinking", AnimationId::Lancer_Drinking },
		{ "Lancer_death", AnimationId::Lancer_Death },
		{ "Paladin_idle", AnimationId::Paladin_Idle },
		{ "Paladin_walk", AnimationId::Paladin_Walk },
		{ "Paladin_run", AnimationId::Paladin_Run },
		{ "Paladin_lightattack1", AnimationId::Paladin_AttackCombo1 },
		{ "Paladin_lightattack2", AnimationId::Paladin_AttackCombo2 },
		{ "Paladin_lightattack3", AnimationId::Paladin_AttackCombo3 },
		{ "Paladin_heavyattack", AnimationId::Paladin_AttackStrong },
		{ "Paladin_specialattack", AnimationId::Paladin_AttackSpecial },
		{ "Paladin_dodge", AnimationId::Paladin_Dodge },
		{ "Paladin_parry", AnimationId::Paladin_Parry },
		{ "Paladin_stun", AnimationId::Paladin_Stun },
		{ "Paladin_hit", AnimationId::Paladin_Hit },
		{ "Paladin_guard", AnimationId::Paladin_Guard },
		{ "Paladin_drinking", AnimationId::Paladin_Drinking },
		{ "Paladin_death", AnimationId::Paladin_Death },

		{ "Imp_death_1", AnimationId::Imp_Death_1 },
		{ "Imp_death_2", AnimationId::Imp_Death_2 },
		{ "Imp_idle_1", AnimationId::Imp_Idle_1 },
		{ "Imp_idle_2", AnimationId::Imp_Idle_2 },
		{ "Imp_idle_3", AnimationId::Imp_Idle_3 },
		{ "Imp_idle_4", AnimationId::Imp_Idle_4 },
		{ "Imp_idle_5", AnimationId::Imp_Idle_5 },
		{ "Imp_idle_6", AnimationId::Imp_Idle_6 },
		{ "Imp_idle_battlecry", AnimationId::Imp_Idle_Battlecry },
		{ "Imp_idle_roaring", AnimationId::Imp_Idle_Roaring },
		{ "Imp_jump_1", AnimationId::Imp_Jump_1 },
		{ "Imp_melee_1", AnimationId::Imp_Melee_1 },
		{ "Imp_melee_2", AnimationId::Imp_Melee_2 },
		{ "Imp_melee_3", AnimationId::Imp_Melee_3 },
		{ "Imp_melee_4", AnimationId::Imp_Melee_4 },
		{ "Imp_melee_5", AnimationId::Imp_Melee_5 },
		{ "Imp_react_front", AnimationId::Imp_React_Front },
		{ "Imp_react_left", AnimationId::Imp_React_Left },
		{ "Imp_react_right", AnimationId::Imp_React_Right },
		{ "Imp_stun", AnimationId::Imp_Stun },
		{ "Imp_walk_back", AnimationId::Imp_Walk_Back },
		{ "Imp_walk_forward", AnimationId::Imp_Walk_Forward },
		{ "Imp_walk_left", AnimationId::Imp_Walk_Left },
		{ "Imp_walk_right", AnimationId::Imp_Walk_Right },

		{ "FinalBoss_idle", AnimationId::FinalBoss_Idle },
		{ "FinalBoss_walk", AnimationId::FinalBoss_Walk },
		{ "FinalBoss_thrust", AnimationId::FinalBoss_Thrust },
		{ "FinalBoss_slash", AnimationId::FinalBoss_Slash },
		{ "FinalBoss_dashslash", AnimationId::FinalBoss_DashSlash },
		{ "FinalBoss_jumpslash", AnimationId::FinalBoss_JumpSlash },
		{ "FinalBoss_multislash", AnimationId::FinalBoss_MultiSlash },
		{ "FinalBoss_bloodlance", AnimationId::FinalBoss_BloodLance },
		{ "FinalBoss_holysandstorm", AnimationId::FinalBoss_HolySandstorm },
		{ "FinalBoss_swordmoonlight", AnimationId::FinalBoss_SwordMoonlight },
		{ "FinalBoss_swordstorm", AnimationId::FinalBoss_SwordStorm },
		{ "FinalBoss_stun", AnimationId::FinalBoss_Stun },
		{ "FinalBoss_hit", AnimationId::FinalBoss_Hit },
		{ "FinalBoss_death", AnimationId::FinalBoss_Death },

		{ "DemonStriker_idle_1", AnimationId::DemonStriker_Idle_1, },
		{ "DemonStriker_idle_2", AnimationId::DemonStriker_Idle_2, },
		{ "DemonStriker_idle_3", AnimationId::DemonStriker_Idle_3, },
		{ "DemonStriker_idle_4", AnimationId::DemonStriker_Idle_4, },
		{ "DemonStriker_idle_5", AnimationId::DemonStriker_Idle_5, },
		{ "DemonStriker_idle_6", AnimationId::DemonStriker_Idle_6_Block, },
		{ "DemonStriker_melee_1", AnimationId::DemonStriker_Melee_1, },
		{ "DemonStriker_melee_2", AnimationId::DemonStriker_Melee_2, },
		{ "DemonStriker_melee_3", AnimationId::DemonStriker_Melee_3, },
		{ "DemonStriker_melee_4", AnimationId::DemonStriker_Melee_4, },
		{ "DemonStriker_gun_shoot_1", AnimationId::DemonStriker_Gun_Shoot_1, },
		{ "DemonStriker_gun_shoot_2", AnimationId::DemonStriker_Gun_Shoot_2, },
		{ "DemonStriker_gun_shoot_3", AnimationId::DemonStriker_Gun_Shoot_3, },
		{ "DemonStriker_gun_shoot_4", AnimationId::DemonStriker_Gun_Shoot_4, },
		{ "DemonStriker_walking_1", AnimationId::DemonStriker_Walking_1, },
		{ "DemonStriker_walking_2", AnimationId::DemonStriker_Walking_2, },
		{ "DemonStriker_walking_3", AnimationId::DemonStriker_Walking_3, },
		{ "DemonStriker_walking_4", AnimationId::DemonStriker_Walking_4_Gun, },
		{ "DemonStriker_walking_5", AnimationId::DemonStriker_Walking_5_Gun, },
		{ "DemonStriker_walking_6", AnimationId::DemonStriker_Walking_6_Block, },
		{ "DemonStriker_walking_back", AnimationId::DemonStriker_Walking_Back, },
		{ "DemonStriker_walking_left", AnimationId::DemonStriker_Walking_Left, },
		{ "DemonStriker_walking_right", AnimationId::DemonStriker_Walking_Right, },
		{ "DemonStriker_turn_left", AnimationId::DemonStriker_Turn_Left, },
		{ "DemonStriker_turn_right", AnimationId::DemonStriker_Turn_Right, },
		{ "DemonStriker_running_1", AnimationId::DemonStriker_Running_1, },
		{ "DemonStriker_running_2", AnimationId::DemonStriker_Running_2, },
		{ "DemonStriker_jump_1", AnimationId::DemonStriker_Jump_1, },
		{ "DemonStriker_jump_2", AnimationId::DemonStriker_Jump_2, },
		{ "DemonStriker_stun", AnimationId::DemonStriker_Stun, },
		{ "DemonStriker_react_1", AnimationId::DemonStriker_React_1_Block, },
		{ "DemonStriker_react_2", AnimationId::DemonStriker_React_2_HeadShot, },
		{ "DemonStriker_react_3", AnimationId::DemonStriker_React_3_Left, },
		{ "DemonStriker_react_4", AnimationId::DemonStriker_React_4_Right, },
		{ "DemonStriker_death_1", AnimationId::DemonStriker_Death_1, },
		{ "DemonStriker_death_2", AnimationId::DemonStriker_Death_2, },

		{ "DemonExecutioner_idle_1", AnimationId::DemonExecutioner_Idle_1, },
		{ "DemonExecutioner_idle_2", AnimationId::DemonExecutioner_Idle_2, },
		{ "DemonExecutioner_idle_3", AnimationId::DemonExecutioner_Idle_3, },
		{ "DemonExecutioner_idle_4", AnimationId::DemonExecutioner_Idle_4, },
		{ "DemonExecutioner_roar_1", AnimationId::DemonExecutioner_Roar_1, },
		{ "DemonExecutioner_roar_2", AnimationId::DemonExecutioner_Roar_2, },
		{ "DemonExecutioner_melee_1", AnimationId::DemonExecutioner_Melee_1, },
		{ "DemonExecutioner_melee_2", AnimationId::DemonExecutioner_Melee_2, },
		{ "DemonExecutioner_melee_3", AnimationId::DemonExecutioner_Melee_3, },
		{ "DemonExecutioner_melee_4", AnimationId::DemonExecutioner_Melee_4, },
		{ "DemonExecutioner_melee_5", AnimationId::DemonExecutioner_Melee_5, },
		{ "DemonExecutioner_melee_6", AnimationId::DemonExecutioner_Melee_6, },
		{ "DemonExecutioner_walk_forward", AnimationId::DemonExecutioner_Walk_Forward, },
		{ "DemonExecutioner_walk_forward_slow", AnimationId::DemonExecutioner_Walk_Forward_Slow, },
		{ "DemonExecutioner_walk_left", AnimationId::DemonExecutioner_Walk_Left, },
		{ "DemonExecutioner_walk_right", AnimationId::DemonExecutioner_Walk_Right, },
		{ "DemonExecutioner_walk_back", AnimationId::DemonExecutioner_Walk_Back, },
		{ "DemonExecutioner_run", AnimationId::DemonExecutioner_Run, },
		{ "DemonExecutioner_jump_1", AnimationId::DemonExecutioner_Jump_1, },
		{ "DemonExecutioner_jump_2", AnimationId::DemonExecutioner_Jump_2, },
		{ "DemonExecutioner_stun", AnimationId::DemonExecutioner_Stun, },
		{ "DemonExecutioner_charge", AnimationId::DemonExecutioner_Charge, },
		{ "DemonExecutioner_block_1", AnimationId::DemonExecutioner_Block_1, },
		{ "DemonExecutioner_block_2", AnimationId::DemonExecutioner_Block_2, },
		{ "DemonExecutioner_get_hit_1", AnimationId::DemonExecutioner_GetHit_1, },
		{ "DemonExecutioner_get_hit_2", AnimationId::DemonExecutioner_GetHit_2, },
		{ "DemonExecutioner_get_hit_3", AnimationId::DemonExecutioner_GetHit_3, },
		{ "DemonExecutioner_death", AnimationId::DemonExecutioner_Death, },

		{ "BigDemonWarrior_idle_1", AnimationId::BigDemonWarrior_Idle_1, },
		{ "BigDemonWarrior_idle_2", AnimationId::BigDemonWarrior_Idle_2, },
		{ "BigDemonWarrior_idle_3", AnimationId::BigDemonWarrior_Idle_3, },
		{ "BigDemonWarrior_idle_4", AnimationId::BigDemonWarrior_Idle_4, },
		{ "BigDemonWarrior_battlecry", AnimationId::BigDemonWarrior_BattleCry, },
		{ "BigDemonWarrior_roaring", AnimationId::BigDemonWarrior_Roaring, },
		{ "BigDemonWarrior_melee_1", AnimationId::BigDemonWarrior_Melee_1, },
		{ "BigDemonWarrior_melee_2", AnimationId::BigDemonWarrior_Melee_2, },
		{ "BigDemonWarrior_melee_3", AnimationId::BigDemonWarrior_Melee_3, },
		{ "BigDemonWarrior_melee_4", AnimationId::BigDemonWarrior_Melee_4, },
		{ "BigDemonWarrior_melee_5", AnimationId::BigDemonWarrior_Melee_5, },
		{ "BigDemonWarrior_melee_6", AnimationId::BigDemonWarrior_Melee_6, },
		{ "BigDemonWarrior_melee_7", AnimationId::BigDemonWarrior_Melee_7, },
		{ "BigDemonWarrior_melee_8", AnimationId::BigDemonWarrior_Melee_8, },
		{ "BigDemonWarrior_walk_back", AnimationId::BigDemonWarrior_WalkBack, },
		{ "BigDemonWarrior_walk_forward", AnimationId::BigDemonWarrior_WalkForward, },
		{ "BigDemonWarrior_walk_left", AnimationId::BigDemonWarrior_WalkLeft, },
		{ "BigDemonWarrior_walk_right", AnimationId::BigDemonWarrior_WalkRight, },
		{ "BigDemonWarrior_run", AnimationId::BigDemonWarrior_RunForward, },
		{ "BigDemonWarrior_turn_left", AnimationId::BigDemonWarrior_TurnLeft, },
		{ "BigDemonWarrior_turn_right", AnimationId::BigDemonWarrior_TurnRight, },
		{ "BigDemonWarrior_jump", AnimationId::BigDemonWarrior_Jump, },
		{ "BigDemonWarrior_stun", AnimationId::BigDemonWarrior_Stun, },
		{ "BigDemonWarrior_react_left", AnimationId::BigDemonWarrior_ReactFromLeft, },
		{ "BigDemonWarrior_react_right", AnimationId::BigDemonWarrior_ReactFromRight, },
		{ "BigDemonWarrior_react_gut", AnimationId::BigDemonWarrior_ReactGut, },
		{ "BigDemonWarrior_death", AnimationId::BigDemonWarrior_Death, },

		{ "Tank_idle_1", AnimationId::Tank_Idle_1, },
		{ "Tank_idle_2", AnimationId::Tank_Idle_2, },
		{ "Tank_idle_3", AnimationId::Tank_Idle_3, },
		{ "Tank_idle_4", AnimationId::Tank_Idle_4, },
		{ "Tank_idle_5", AnimationId::Tank_Idle_5, },
		{ "Tank_melee_1", AnimationId::Tank_Melee_1, },
		{ "Tank_melee_2", AnimationId::Tank_Melee_2, },
		{ "Tank_melee_3", AnimationId::Tank_Melee_3, },
		{ "Tank_melee_4", AnimationId::Tank_Melee_4, },
		{ "Tank_melee_5", AnimationId::Tank_Melee_5, },
		{ "Tank_melee_6", AnimationId::Tank_Melee_6, },
		{ "Tank_melee_7", AnimationId::Tank_Melee_7, },
		{ "Tank_melee_8", AnimationId::Tank_Melee_8, },
		{ "Tank_walk_2", AnimationId::Tank_Walk_2, },
		{ "Tank_walk_back", AnimationId::Tank_WalkBack, },
		{ "Tank_walk_left", AnimationId::Tank_WalkLeft, },
		{ "Tank_walk_left_back", AnimationId::Tank_WalkLeftBack, },
		{ "Tank_walk_right", AnimationId::Tank_WalkRight, },
		{ "Tank_walk_right_back", AnimationId::Tank_WalkRightBack, },
		{ "Tank_turn_left", AnimationId::Tank_TurnLeft, },
		{ "Tank_turn_right", AnimationId::Tank_TurnRight, },
		{ "Tank_run_1", AnimationId::Tank_Running1, },
		{ "Tank_run_2", AnimationId::Tank_Running2, },
		{ "Tank_jump_1", AnimationId::Tank_Jump1, },
		{ "Tank_jump_2", AnimationId::Tank_Jump2, },
		{ "Tank_stun", AnimationId::Tank_Stun, },
		{ "Tank_death_1", AnimationId::Tank_Death1, },
		{ "Tank_death_2", AnimationId::Tank_Death2, },
	};

	const auto it = kClipIdToAnimationId.find(clipId);
	if (it == kClipIdToAnimationId.end())
	{
		outError = "Failed to resolve AnimationId from clipId: " + std::string(clipId);
		return false;
	}

	outId = it->second;
	return true;
}

bool AnimationJsonLoader::ParseCapsuleRole(
	std::string_view roleText,
	CapsuleRole& outRole,
	std::string& outError) const
{
	if (roleText == "hit")
	{
		outRole = CapsuleRole::Hit;
		return true;
	}

	if (roleText == "hurt")
	{
		outRole = CapsuleRole::Hurt;
		return true;
	}

	if (roleText == "guard")
	{
		outRole = CapsuleRole::Guard;
		return true;
	}

	if (roleText == "parry")
	{
		outRole = CapsuleRole::Parry;
		return true;
	}

	outError = "Unsupported capsule role: " + std::string(roleText);
	return false;
}

bool AnimationJsonLoader::ValidateParsedDef(
	const AnimationClipDef& def,
	std::string& outError) const
{
	if (def.version != 3)
	{
		outError = "Animation json version must be 3.";
		return false;
	}

	if (def.fps <= 0.0f)
	{
		outError = "Animation fps must be greater than zero.";
		return false;
	}

	if (def.numFrames != def.frames.size())
	{
		outError = "Animation numFrames does not match parsed frame count.";
		return false;
	}

	for (const auto& frame : def.frames)
	{
		if (frame.capsules.size() != def.capsuleDefs.size())
		{
			outError = "Animation frame capsule count does not match capsuleDefs.";
			return false;
		}
	}

	return true;
}
