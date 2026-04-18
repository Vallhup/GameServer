#pragma once

#include <cstdint>

enum class BuffId : uint8_t
{
    None = 0,
    HpBoost_Low = 1,
    HpBoost_Mid = 2,
    HpBoost_High = 3,
    StaminaBoost = 4,
    AttackBoost = 5,
    AttackSpeedBoost = 6,
    DefenceBoost = 7,
    MoveSpeedBoost = 8,
    ParrySuccess = 9,
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

enum class GameplayStateFlag : uint16_t
{
    None = 0,

    SuperArmor,
    Invulnerable,
    CannotMove,
    CannotAct,

    ParrySuccessReady
};

enum class ActionId : uint16_t
{
    None,

    /* [ Knight ] */
    Knight_LightAttack1,
    Knight_LightAttack2,
    Knight_LightAttack3,
    Knight_HeavyAttack,
    Knight_SpecialAttack,
    Knight_Dodge,
    Knight_Parry,
    Knight_Stun,
    Knight_Hit,
    Knight_Guard,
    Knight_UseHpPotion,
    Knight_Dead,

    /* [ Lancer ] */
    Lancer_LightAttack1,
    Lancer_LightAttack2,
    Lancer_LightAttack3,
    Lancer_HeavyAttack,
    Lancer_SpecialAttack,
    Lancer_Dodge,
    Lancer_Parry,
    Lancer_Stun,
    Lancer_Hit,
    Lancer_Guard,
    Lancer_UseHpPotion,
    Lancer_Dead,

    /* [ Paladin ] */
    Paladin_LightAttack1,
    Paladin_LightAttack2,
    Paladin_LightAttack3,
    Paladin_HeavyAttack,
    Paladin_SpecialAttack,
    Paladin_Dodge,
    Paladin_Parry,
    Paladin_Stun,
    Paladin_Hit,
    Paladin_Guard,
    Paladin_UseHpPotion,
    Paladin_Dead,

    /* [ Imp ] */
    Imp_melee1,
    Imp_melee2,
    Imp_melee3,
    Imp_melee4,
    Imp_melee5,
    Imp_Jump,
    Imp_Stun,
    Imp_Hit,
    Imp_Dead,

    /* [ Demon Striker ] */
    DemonStriker_Melee_1,
    DemonStriker_Melee_2,
    DemonStriker_Melee_3,
    DemonStriker_Melee_4,
    DemonStriker_Gun_Shoot_1,
    DemonStriker_Gun_Shoot_2,
    DemonStriker_Gun_Shoot_3,
    DemonStriker_Gun_Shoot_4,
    DemonStriker_Jump_1,
    DemonStriker_Jump_2,
    DemonStriker_Stun,
    DemonStriker_Hit,
    DemonStriker_Dead,

    /* [ Demon Executioner ] */
    DemonExecutioner_Melee_1,
    DemonExecutioner_Melee_2,
    DemonExecutioner_Melee_3,
    DemonExecutioner_Melee_4,
    DemonExecutioner_Melee_5,
    DemonExecutioner_Melee_6,
    DemonExecutioner_Jump_1,
    DemonExecutioner_Jump_2,
    DemonExecutioner_Stun,
    DemonExecutioner_Hit,
    DemonExecutioner_Dead,


    /* [ DemonWarrior ] */

    /* [ Tank ] */

    FinalBoss_Thrust,   
    FinalBoss_Slash,
    FinalBoss_DashSlash,
    FinalBoss_JumpSlash,
    FinalBoss_MultiSlash,
    FinalBoss_Meteor,
    FinalBoss_Gimmic1,
    FinalBoss_Gimmic2,
    FinalBoss_Stun,
    FinalBoss_Hit,
    FinalBoss_Dead,
};
