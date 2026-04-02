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
		{ "Knight_attack", AnimationId::Knight_Attack },
		{ "Knight_dodge", AnimationId::Knight_Dodge },
		{ "Knight_parry", AnimationId::Knight_Parry },
		{ "Knight_stun", AnimationId::Knight_Stun },
		{ "Knight_hit", AnimationId::Knight_Hit },
		{ "Knight_guard", AnimationId::Knight_Guard },
		{ "Knight_drinking", AnimationId::Knight_Drinking },
		{ "Knight_death", AnimationId::Knight_Dead },

		{ "Imp_idle_1", AnimationId::Imp_Idle },
		{ "Imp_walk_forward", AnimationId::Imp_Walk },
		{ "Imp_melee_1", AnimationId::Imp_melee1 },
		{ "Imp_melee_2", AnimationId::Imp_melee2 },
		{ "Imp_melee_3", AnimationId::Imp_melee3 },
		{ "Imp_melee_4", AnimationId::Imp_melee4 },
		{ "Imp_melee_5", AnimationId::Imp_melee5 },
		{ "Imp_stun", AnimationId::Imp_Stun },
		{ "Imp_hit", AnimationId::Imp_Hit },
		{ "Imp_death_1", AnimationId::Imp_Dead },

		{ "FinalBoss_idle", AnimationId::FinalBoss_Idle },
		{ "FinalBoss_walk", AnimationId::FinalBoss_Walk },
		{ "FinalBoss_thrust", AnimationId::FinalBoss_Thrust },
		{ "FinalBoss_slash", AnimationId::FinalBoss_Slash },
		{ "FinalBoss_dashslash", AnimationId::FinalBoss_DashSlash },
		{ "FinalBoss_jumpslash", AnimationId::FinalBoss_JumpSlash },
		{ "FinalBoss_multislash", AnimationId::FinalBoss_MultiSlash },
		{ "FinalBoss_stun", AnimationId::FinalBoss_Stun },
		{ "FinalBoss_hit", AnimationId::FinalBoss_Hit },
		{ "FinalBoss_death", AnimationId::FinalBoss_Dead },
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
