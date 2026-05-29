#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "DefLoadResult.h"
#include "DefRegistry.h"
#include "EntityId.h"

enum class CharacterId : uint8_t;

enum class TitleConditionType : uint8_t
{
    None           = 0,
    MonsterKill    = 1,
    DeathByMonster = 2,
};

using TitleId = uint16_t;
inline constexpr TitleId InvalidTitleId = 0;

struct TitleDef
{
    TitleId             id{ InvalidTitleId };
    std::string         displayName;
    TitleConditionType  conditionType{ TitleConditionType::None };
    CharacterId         characterId{};
    int32_t             requiredCount{ 0 };
};

struct TitleDefTraits
{
    static TitleId GetId(const TitleDef& def) noexcept
    {
        return def.id;
    }
};

using TitleDefRegistry = DefRegistry<TitleDef, TitleId, TitleDefTraits>;

using TitleDefLoadResult = DefLoadResult;

TitleDefLoadResult LoadTitleDefsFromJsonDirectory(
    const std::filesystem::path& directory,
    TitleDefRegistry& outRegistry);
