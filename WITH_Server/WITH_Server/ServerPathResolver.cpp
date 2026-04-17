#include "pch.h"
#include "ServerPathResolver.h"

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

std::filesystem::path ServerPathResolver::GetExecutableDirectory()
{
//#if defined(_WIN32)
//	wchar_t* programPath = nullptr;
//	if (_get_wpgmptr(&programPath) == 0 &&
//		programPath != nullptr &&
//		programPath[0] != L'\0')
//	{
//		return NormalizePath(std::filesystem::path(programPath).parent_path());
//	}
//#endif

	return NormalizePath(std::filesystem::current_path());
}

std::filesystem::path ServerPathResolver::GetDefaultAnimationOutputRoot()
{
	const std::filesystem::path exeDir = GetExecutableDirectory();
	const std::filesystem::path currentDir =
		NormalizePath(std::filesystem::current_path());

	const std::vector<std::filesystem::path> candidates =
	{
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
		root / "Knight" / "knight_animation_heavyattack.json",
		root / "Knight" / "knight_animation_hit.json",
		root / "Knight" / "knight_animation_idle.json",
		root / "Knight" / "knight_animation_lightattack1.json",
		root / "Knight" / "knight_animation_lightattack2.json",
		root / "Knight" / "knight_animation_lightattack3.json",
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
	};
}
