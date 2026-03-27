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
};

enum class StatType : uint8_t
{
    MaxHp,
    MaxStamina,
    AttackPower,
    Defense,
    MoveSpeed,
    AttackSpeed
};

enum class StateFlagType : uint8_t
{
    SuperArmor,
    Invulnerable,
    CannotMove,
    CannotAct
};