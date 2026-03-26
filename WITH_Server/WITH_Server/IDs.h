#pragma once

#include <cstdint>
#include "EntityId.h"

enum class BuffId : uint8_t
{
    None = 0,
    HpBoost = 1,
    StaminaBoost = 2,
    AttackBoost = 3,
    AttackSpeedBoost = 4,
    DefenceBoost = 5,
    MoveSpeedBoost = 6,
};

enum class ActionId : uint8_t
{
    None = 0,
    Attack = 1,
    Dodge = 2,
    Parry = 3,
    Stun = 4,
    Hit = 5,
    Guard = 6,
    Dead = 7,
};

enum class WorldId : uint8_t
{
    None = 0,
    Square = 1,
    Knight_Start = 2,
    Lancer_Start = 3,
    ThirdCharacter_Start = 4,
    Middle = 5,
    Final = 6,
    Pvp = 7
};

enum class SpawnSetId : uint8_t
{
    None = 0,
    SquareDefault = 1,
    KnightStartDefault = 2,
    LancerStartDefault = 3,
    ThirdCharacterStartDefault = 4,
    MiddleDefault = 5,
    FinalDefault = 6,
    PvpDefault = 7,
};

struct ActionKey
{
    CharacterId characterId{ CharacterId::None };
    uint8_t actionLocalId{ 0 };

    bool operator==(const ActionKey&) const noexcept = default;
};

namespace std
{
    template<>
    struct hash<ActionKey>
    {
        size_t operator()(const ActionKey& key) const noexcept
        {
            return (static_cast<size_t>(key.characterId) << 8) ^
                static_cast<size_t>(key.actionLocalId);
        }
    };
}