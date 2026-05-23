#pragma once

#include <filesystem>
#include <vector>

class ServerPathResolver final {
public:
	static std::filesystem::path NormalizePath(const std::filesystem::path& path);
	static std::filesystem::path GetExecutableDirectory();
	static std::filesystem::path GetDefaultAttributeDefRoot();
	static std::filesystem::path GetDefaultGameplayTagDefRoot();
	static std::filesystem::path GetDefaultGameplayEffectDefRoot();
	static std::filesystem::path GetDefaultProjectileDefRoot();
	static std::filesystem::path GetDefaultAreaHitDefRoot();
	static std::filesystem::path GetDefaultAbilityDefRoot();
	static std::filesystem::path GetDefaultAbilitySetDefRoot();
	static std::filesystem::path GetDefaultCharacterDefRoot();
	static std::filesystem::path GetDefaultAIBehaviorDefRoot();
	static std::filesystem::path GetDefaultSpawnSetDefRoot();
	static std::filesystem::path GetDefaultAnimationOutputRoot();
	static std::vector<std::filesystem::path> GetBootAnimationCandidates(
		const std::filesystem::path& root);

private:
	static bool IsDirectory(const std::filesystem::path& path);
	static std::filesystem::path GetDefaultDataRoot(
		const char* relativeDataDirectory);
};
