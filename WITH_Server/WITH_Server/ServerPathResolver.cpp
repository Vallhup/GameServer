#include "pch.h"
#include "ServerPathResolver.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace
{
	std::filesystem::path GetProcessExecutablePath()
	{
#ifdef _WIN32
		std::vector<wchar_t> buffer(MAX_PATH);

		for (;;)
		{
			const DWORD length = GetModuleFileNameW(
				nullptr,
				buffer.data(),
				static_cast<DWORD>(buffer.size()));

			if (length == 0)
				break;

			if (length < buffer.size() - 1)
				return std::filesystem::path(buffer.data());

			buffer.resize(buffer.size() * 2);
		}
#endif

		return std::filesystem::current_path();
	}
}

std::filesystem::path ServerPathResolver::NormalizePath(
	const std::filesystem::path& path)
{
	std::error_code ec;
	std::filesystem::path normalized = std::filesystem::absolute(path, ec);
	if (ec)
	{
		normalized = path;
	}
	return normalized.lexically_normal();
}

bool ServerPathResolver::IsDirectory(const std::filesystem::path& path)
{
	std::error_code ec;
	return std::filesystem::exists(path, ec) &&
		std::filesystem::is_directory(path, ec);
}

std::filesystem::path ServerPathResolver::GetDefaultDataRoot(
	const char* relativeDataDirectory)
{
	const std::filesystem::path exeDir =
		GetExecutableDirectory();
	const std::filesystem::path currentDir =
		NormalizePath(std::filesystem::current_path());

	const std::vector<std::filesystem::path> candidates =
	{
		exeDir / ".." / "Data" / relativeDataDirectory,
		currentDir / "WITH_Server" / "Data" / relativeDataDirectory,
		currentDir / "Data" / relativeDataDirectory,
		currentDir / ".." / "Data" / relativeDataDirectory,
		exeDir / "Data" / relativeDataDirectory,
		exeDir / ".." / ".." / "Data" / relativeDataDirectory,
		exeDir / ".." / ".." / "WITH_Server" / "Data" / relativeDataDirectory
	};

	for (const std::filesystem::path& candidate : candidates)
	{
		if (std::filesystem::exists(candidate) &&
			std::filesystem::is_directory(candidate))
		{
			return NormalizePath(candidate);
		}
	}

	return NormalizePath(
		currentDir / "WITH_Server" / "Data" / relativeDataDirectory);
}

std::filesystem::path ServerPathResolver::GetExecutableDirectory()
{
	return NormalizePath(GetProcessExecutablePath()).parent_path();
}

std::filesystem::path ServerPathResolver::GetDefaultAttributeDefRoot()
{
	return GetDefaultDataRoot("Attribute");
}

std::filesystem::path ServerPathResolver::GetDefaultGameplayTagDefRoot()
{
	return GetDefaultDataRoot("Tag");
}

std::filesystem::path ServerPathResolver::GetDefaultGameplayEffectDefRoot()
{
	return GetDefaultDataRoot("Effect");
}

std::filesystem::path ServerPathResolver::GetDefaultAbilityDefRoot()
{
	return GetDefaultDataRoot("Ability");
}

std::filesystem::path ServerPathResolver::GetDefaultAbilitySetDefRoot()
{
	return GetDefaultDataRoot("AbilitySet");
}

std::filesystem::path ServerPathResolver::GetDefaultCharacterDefRoot()
{
	const std::filesystem::path exeDir = GetExecutableDirectory();
	const std::filesystem::path currentDir =
		NormalizePath(std::filesystem::current_path());

	const std::vector<std::filesystem::path> candidates =
	{
		exeDir / ".." / "Data" / "Character",
		currentDir / "WITH_Server" / "Data" / "Character",
		currentDir / "Data" / "Character",
		currentDir / ".." / "Data" / "Character",
		exeDir / "Data" / "Character",
		exeDir / ".." / ".." / "Data" / "Character",
		exeDir / ".." / ".." / "WITH_Server" / "Data" / "Character"
	};

	for (const std::filesystem::path& candidate : candidates)
	{
		if (IsDirectory(candidate))
			return NormalizePath(candidate);
	}

	return NormalizePath(currentDir / "WITH_Server" / "Data" / "Character");
}

std::filesystem::path ServerPathResolver::GetDefaultAIBehaviorDefRoot()
{
	const std::filesystem::path exeDir = GetExecutableDirectory();
	const std::filesystem::path currentDir =
		NormalizePath(std::filesystem::current_path());

	const std::vector<std::filesystem::path> candidates =
	{
		exeDir / ".." / "Data" / "AI",
		currentDir / "WITH_Server" / "Data" / "AI",
		currentDir / "Data" / "AI",
		currentDir / ".." / "Data" / "AI",
		exeDir / "Data" / "AI",
		exeDir / ".." / ".." / "Data" / "AI",
		exeDir / ".." / ".." / "WITH_Server" / "Data" / "AI"
	};

	for (const std::filesystem::path& candidate : candidates)
	{
		if (IsDirectory(candidate))
			return NormalizePath(candidate);
	}

	return NormalizePath(currentDir / "WITH_Server" / "Data" / "AI");
}

std::filesystem::path ServerPathResolver::GetDefaultSpawnSetDefRoot()
{
	const std::filesystem::path exeDir = GetExecutableDirectory();
	const std::filesystem::path currentDir =
		NormalizePath(std::filesystem::current_path());

	const std::vector<std::filesystem::path> candidates =
	{
		exeDir / ".." / "Data" / "SpawnSet",
		currentDir / "WITH_Server" / "Data" / "SpawnSet",
		currentDir / "Data" / "SpawnSet",
		currentDir / ".." / "Data" / "SpawnSet",
		exeDir / "Data" / "SpawnSet",
		exeDir / ".." / ".." / "Data" / "SpawnSet",
		exeDir / ".." / ".." / "WITH_Server" / "Data" / "SpawnSet"
	};

	for (const std::filesystem::path& candidate : candidates)
	{
		if (IsDirectory(candidate))
			return NormalizePath(candidate);
	}

	return NormalizePath(currentDir / "WITH_Server" / "Data" / "SpawnSet");
}

std::filesystem::path ServerPathResolver::GetDefaultAnimationOutputRoot()
{
	const std::filesystem::path exeDir = GetExecutableDirectory();
	const std::filesystem::path currentDir =
		NormalizePath(std::filesystem::current_path());

	const std::vector<std::filesystem::path> candidates =
	{
		exeDir / ".." / "Animation",
		exeDir / ".." / ".." / "Animation",
		exeDir / "Animation",
		currentDir / "Animation",
		currentDir / ".." / "Animation",
		currentDir / ".." / ".." / "Animation",
	};

	for (const std::filesystem::path& candidate : candidates)
	{
		if (IsDirectory(candidate))
		{
			return NormalizePath(candidate);
		}
	}

	return NormalizePath(exeDir / ".." / ".." / "Animation");
}

std::vector<std::filesystem::path> ServerPathResolver::GetBootAnimationCandidates(
	const std::filesystem::path& root)
{
	return
	{
		root / "Imp" / "imp_animation_death_1.json",
		root / "Imp" / "imp_animation_death_2.json",
		root / "Imp" / "imp_animation_idle_1.json",
		root / "Imp" / "imp_animation_idle_2.json",
		root / "Imp" / "imp_animation_idle_3.json",
		root / "Imp" / "imp_animation_idle_4.json",
		root / "Imp" / "imp_animation_idle_5.json",
		root / "Imp" / "imp_animation_idle_6.json",
		root / "Imp" / "imp_animation_idle_battlecry.json",
		root / "Imp" / "imp_animation_idle_roaring.json",
		root / "Imp" / "imp_animation_jump_1.json",
		root / "Imp" / "imp_animation_melee_1.json",
		root / "Imp" / "imp_animation_melee_2.json",
		root / "Imp" / "imp_animation_melee_3.json",
		root / "Imp" / "imp_animation_melee_4.json",
		root / "Imp" / "imp_animation_melee_5.json",
		root / "Imp" / "imp_animation_react_front.json",
		root / "Imp" / "imp_animation_react_left.json",
		root / "Imp" / "imp_animation_react_right.json",
		root / "Imp" / "imp_animation_stun.json",
		root / "Imp" / "imp_animation_walk_back.json",
		root / "Imp" / "imp_animation_walk_forward.json",
		root / "Imp" / "imp_animation_walk_left.json",
		root / "Imp" / "imp_animation_walk_right.json",

		root / "Knight" / "knight_animation_death.json",
		root / "Knight" / "knight_animation_dodge.json",
		root / "Knight" / "knight_animation_drinking.json",
		root / "Knight" / "knight_animation_guard.json",
		root / "Knight" / "knight_animation_hit.json",
		root / "Knight" / "knight_animation_idle.json",
		root / "Knight" / "knight_animation_attack_combo01.json",
		root / "Knight" / "knight_animation_attack_combo02.json",
		root / "Knight" / "knight_animation_attack_combo03.json",
		root / "Knight" / "knight_animation_attack_strong.json",
		root / "Knight" / "knight_animation_parry.json",
		root / "Knight" / "knight_animation_run.json",
		root / "Knight" / "knight_animation_specialattack.json",
		root / "Knight" / "knight_animation_stun.json",
		root / "Knight" / "knight_animation_walk.json",

		root / "Final_Boss" / "final_boss_animation_dashslash.json",
		root / "Final_Boss" / "final_boss_animation_death.json",
		root / "Final_Boss" / "final_boss_animation_hit.json",
		root / "Final_Boss" / "final_boss_animation_idle.json",
		root / "Final_Boss" / "final_boss_animation_jumpslash.json",
		root / "Final_Boss" / "final_boss_animation_multislash.json",
		root / "Final_Boss" / "final_boss_animation_slash.json",
		root / "Final_Boss" / "final_boss_animation_stun.json",
		root / "Final_Boss" / "final_boss_animation_thrust.json",
		root / "Final_Boss" / "final_boss_animation_walk.json",

		root / "DemonStriker" / "demonstriker_animation_death_1.json",
		root / "DemonStriker" / "demonstriker_animation_death_2.json",
		root / "DemonStriker" / "demonstriker_animation_gun_shoot_1.json",
		root / "DemonStriker" / "demonstriker_animation_gun_shoot_2.json",
		root / "DemonStriker" / "demonstriker_animation_gun_shoot_3.json",
		root / "DemonStriker" / "demonstriker_animation_gun_shoot_4.json",
		root / "DemonStriker" / "demonstriker_animation_idle_1.json",
		root / "DemonStriker" / "demonstriker_animation_idle_2.json",
		root / "DemonStriker" / "demonstriker_animation_idle_3.json",
		root / "DemonStriker" / "demonstriker_animation_idle_4.json",
		root / "DemonStriker" / "demonstriker_animation_idle_5.json",
		root / "DemonStriker" / "demonstriker_animation_idle_6.json",
		root / "DemonStriker" / "demonstriker_animation_jump_1.json",
		root / "DemonStriker" / "demonstriker_animation_jump_2.json",
		root / "DemonStriker" / "demonstriker_animation_melee_1.json",
		root / "DemonStriker" / "demonstriker_animation_melee_2.json",
		root / "DemonStriker" / "demonstriker_animation_melee_3.json",
		root / "DemonStriker" / "demonstriker_animation_melee_4.json",
		root / "DemonStriker" / "demonstriker_animation_react_1.json",
		root / "DemonStriker" / "demonstriker_animation_react_2.json",
		root / "DemonStriker" / "demonstriker_animation_react_3.json",
		root / "DemonStriker" / "demonstriker_animation_react_4.json",
		root / "DemonStriker" / "demonstriker_animation_running_1.json",
		root / "DemonStriker" / "demonstriker_animation_running_2.json",
		root / "DemonStriker" / "demonstriker_animation_stun.json",
		root / "DemonStriker" / "demonstriker_animation_turn_left.json",
		root / "DemonStriker" / "demonstriker_animation_turn_right.json",
		root / "DemonStriker" / "demonstriker_animation_walking_1.json",
		root / "DemonStriker" / "demonstriker_animation_walking_2.json",
		root / "DemonStriker" / "demonstriker_animation_walking_3.json",
		root / "DemonStriker" / "demonstriker_animation_walking_4.json",
		root / "DemonStriker" / "demonstriker_animation_walking_5.json",
		root / "DemonStriker" / "demonstriker_animation_walking_6.json",
		root / "DemonStriker" / "demonstriker_animation_walking_back.json",
		root / "DemonStriker" / "demonstriker_animation_walking_left.json",
		root / "DemonStriker" / "demonstriker_animation_walking_right.json",

		root / "DemonExecutioner" / "demonexecutioner_animation_block_1.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_block_2.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_charge.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_death.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_get_hit_1.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_get_hit_2.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_get_hit_3.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_idle_1.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_idle_2.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_idle_3.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_idle_4.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_jump_1.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_jump_2.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_melee_1.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_melee_2.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_melee_3.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_melee_4.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_melee_5.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_melee_6.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_roar_1.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_roar_2.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_run.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_stun.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_walk_back.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_walk_forward.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_walk_forward_slow.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_walk_left.json",
		root / "DemonExecutioner" / "demonexecutioner_animation_walk_right.json",

		root / "BigDemonWarrior" / "bigdemonwarrior_animation_battlecry.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_death.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_idle_1.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_idle_2.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_idle_3.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_idle_4.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_jump.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_melee_1.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_melee_2.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_melee_3.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_melee_4.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_melee_5.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_melee_6.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_melee_7.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_melee_8.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_react_gut.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_react_left.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_react_right.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_roaring.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_run.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_stun.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_turn_left.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_turn_right.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_walk_back.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_walk_forward.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_walk_left.json",
		root / "BigDemonWarrior" / "bigdemonwarrior_animation_walk_right.json",

		root / "Tank" / "tank_animation_death_1.json",
		root / "Tank" / "tank_animation_death_2.json",
		root / "Tank" / "tank_animation_idle_1.json",
		root / "Tank" / "tank_animation_idle_2.json",
		root / "Tank" / "tank_animation_idle_3.json",
		root / "Tank" / "tank_animation_idle_4.json",
		root / "Tank" / "tank_animation_idle_5.json",
		root / "Tank" / "tank_animation_jump_1.json",
		root / "Tank" / "tank_animation_jump_2.json",
		root / "Tank" / "tank_animation_melee_1.json",
		root / "Tank" / "tank_animation_melee_2.json",
		root / "Tank" / "tank_animation_melee_3.json",
		root / "Tank" / "tank_animation_melee_4.json",
		root / "Tank" / "tank_animation_melee_5.json",
		root / "Tank" / "tank_animation_melee_6.json",
		root / "Tank" / "tank_animation_melee_7.json",
		root / "Tank" / "tank_animation_melee_8.json",
		root / "Tank" / "tank_animation_run_1.json",
		root / "Tank" / "tank_animation_run_2.json",
		root / "Tank" / "tank_animation_stun.json",
		root / "Tank" / "tank_animation_turn_left.json",
		root / "Tank" / "tank_animation_turn_right.json",
		root / "Tank" / "tank_animation_walk_1.json",
		root / "Tank" / "tank_animation_walk_2.json",
		root / "Tank" / "tank_animation_walk_back.json",
		root / "Tank" / "tank_animation_walk_left.json",
		root / "Tank" / "tank_animation_walk_left_back.json",
		root / "Tank" / "tank_animation_walk_right.json",

		root / "Lancer" / "lancer_animation_death.json",
		root / "Lancer" / "lancer_animation_dodge.json",
		root / "Lancer" / "lancer_animation_drinking.json",
		root / "Lancer" / "lancer_animation_guard.json",
		root / "Lancer" / "lancer_animation_heavyattack.json",
		root / "Lancer" / "lancer_animation_hit.json",
		root / "Lancer" / "lancer_animation_idle.json",
		root / "Lancer" / "lancer_animation_lightattack1.json",
		root / "Lancer" / "lancer_animation_lightattack2.json",
		root / "Lancer" / "lancer_animation_lightattack3.json",
		root / "Lancer" / "lancer_animation_parry.json",
		root / "Lancer" / "lancer_animation_run.json",
		root / "Lancer" / "lancer_animation_specialattack.json",
		root / "Lancer" / "lancer_animation_stun_left.json",
		root / "Lancer" / "lancer_animation_stun_right.json",
		root / "Lancer" / "lancer_animation_walk.json",

		root / "Paladin" / "paladin_animation_death.json",
		root / "Paladin" / "paladin_animation_dodge.json",
		root / "Paladin" / "paladin_animation_drinking.json",
		root / "Paladin" / "paladin_animation_guard.json",
		root / "Paladin" / "paladin_animation_heavyattack.json",
		root / "Paladin" / "paladin_animation_hit.json",
		root / "Paladin" / "paladin_animation_idle.json",
		root / "Paladin" / "paladin_animation_lightattack1.json",
		root / "Paladin" / "paladin_animation_lightattack2.json",
		root / "Paladin" / "paladin_animation_lightattack3.json",
		root / "Paladin" / "paladin_animation_parry.json",
		root / "Paladin" / "paladin_animation_run.json",
		root / "Paladin" / "paladin_animation_specialattack.json",
		root / "Paladin" / "paladin_animation_stun.json",
		root / "Paladin" / "paladin_animation_walk.json",
	};
}
