#pragma once

#include "DefLoadResult.h"
#include "DefRegistry.h"
#include "GameplayContentIds.h"

#include <filesystem>
#include <span>
#include <string>
#include <vector>

struct GameplayTagDef
{
	GameplayTagId id{ InvalidGameplayTagId };
	std::string key;
	std::string name;
	std::string path;
	std::vector<GameplayTagId> parents;
};

enum class GameplayTagQueryOp : uint8_t
{
	None,
	All,
	Any,
	Not
};

struct GameplayTagQueryDef
{
	GameplayTagQueryOp op{ GameplayTagQueryOp::None };
	std::vector<GameplayTagId> tags;
	std::vector<GameplayTagQueryDef> children;
};

bool ValidateGameplayTagDefs(
	std::span<const GameplayTagDef> defs,
	std::string& outError);

struct GameplayTagDefTraits
{
	static GameplayTagId GetId(const GameplayTagDef& def) noexcept
	{
		return def.id;
	}
};

using GameplayTagDefRegistry = DefRegistry<
	GameplayTagDef,
	GameplayTagId,
	GameplayTagDefTraits>;

DefLoadResult LoadGameplayTagDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	GameplayTagDefRegistry& outRegistry);
