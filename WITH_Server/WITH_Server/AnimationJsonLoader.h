#pragma once

#include "AnimationDef.h"
#include <filesystem>
#include <string>
#include <vector>

class AnimationJsonLoader
{
public:
	struct LoadResult
	{
		bool succeeded = false;
		std::string error;
		size_t loadedCount = 0;
	};

public:
	LoadResult LoadFile(
		const std::filesystem::path& path,
		AnimationClipDef& outDef) const;

	LoadResult LoadDirectory(
		const std::filesystem::path& directory,
		std::vector<AnimationClipDef>& outDefs) const;

private:
	bool ParseDocument(
		const std::string& jsonText,
		AnimationClipDef& outDef,
		std::string& outError) const;

	bool ResolveAnimationId(
		std::string_view clipId,
		AnimationId& outId,
		std::string& outError) const;

	bool ParseCapsuleRole(
		std::string_view roleText,
		CapsuleRole& outRole,
		std::string& outError) const;

	bool ValidateParsedDef(
		const AnimationClipDef& def,
		std::string& outError) const;
};
