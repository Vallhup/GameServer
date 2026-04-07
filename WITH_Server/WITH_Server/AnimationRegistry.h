#pragma once

#include "AnimationDef.h"
#include "DefRegistry.h"
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class AnimationRegistry {
public:
	struct BuildResult
	{
		bool succeeded = false;
		std::string error;
		size_t loadedCount = 0;
	};

public:
	AnimationRegistry() = default;

	BuildResult Rebuild(std::vector<AnimationClipDef> defs);
	void Clear() noexcept;

	bool Empty() const noexcept;
	size_t Size() const noexcept;

	const AnimationClipDef* Find(AnimationId id) const noexcept;
	const AnimationClipDef& Get(AnimationId id) const;

	const AnimationClipDef* FindByClipId(std::string_view clipId) const noexcept;
	std::span<const AnimationClipDef> GetAll() const noexcept;

	std::vector<const AnimationClipDef*> FindBySkeleton(std::string_view skeleton) const;

private:
	struct Traits
	{
		static AnimationId GetId(const AnimationClipDef& def) noexcept
		{
			return def.id;
		}
	};

private:
	static bool ValidateDef(const AnimationClipDef& def, std::string& outError);

private:
	DefRegistry<AnimationClipDef, AnimationId, Traits> _defs;
	std::unordered_map<std::string, AnimationId> _clipIdToId;
	std::unordered_map<std::string, std::vector<AnimationId>> _skeletonToIds;
};
