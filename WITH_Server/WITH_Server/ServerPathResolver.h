#pragma once

#include <filesystem>
#include <vector>

class ServerPathResolver final {
public:
	static std::filesystem::path NormalizePath(const std::filesystem::path& path);
	static std::filesystem::path GetExecutableDirectory();
	static std::filesystem::path GetDefaultActionDefRoot();
	static std::filesystem::path GetDefaultCharacterDefRoot();
	static std::filesystem::path GetDefaultAIBehaviorDefRoot();
	static std::filesystem::path GetDefaultBuffDefRoot();
	static std::filesystem::path GetDefaultSpawnSetDefRoot();
	static std::filesystem::path GetDefaultAnimationOutputRoot();
	static std::vector<std::filesystem::path> GetBootAnimationCandidates(
		const std::filesystem::path& root);

private:
	static bool IsDirectory(const std::filesystem::path& path);
};
