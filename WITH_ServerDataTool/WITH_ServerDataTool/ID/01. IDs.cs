using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WITH_ServerDataTool.ID
{
    /* [ Category ID ] */
    public enum EntityCategoryId : byte
    {
        None            = 0,
        Character       = 1,
        Interactable    = 2,
        WorldObject     = 3,
    }

    public enum FactionId : byte
    {
        None    = 0,
        Neutral = 1,
        Player  = 2,
        Enemy   = 3,
    }


    /* [ Contents ID ] */
    public enum CharacterId : byte
    {
        None            = 0,

        // Playable
        Knight          = 1,
        Lancer          = 2,


        // Monster
        Imp             = 100,
        DemonWarrior    = 101,
        Tank            = 102,
        FinalBoss       = 103,
    }

    public enum AIArchetypeId : byte
    {
        None                = 0,
        Humanoid            = 1,
        NormalMonster       = 2,
        FirstBossMonster    = 3,
        MidBossMonster      = 4,
        FinalBossMonster    = 5,
    }

    public enum BuffId : byte
    {
        None                = 0,
        HpBoost             = 1,
        StaminaBoost        = 2,
        AttackBoost         = 3,
        AttackSpeedBoost    = 4,
        DefenceBoost        = 5,
        MoveSpeedBoost      = 6,
    }


    /* [ Logic ID ] */
    public enum ActionId : ushort
    {
        None = 0,

        Knight_Light_Attack1    = 1,
        Knight_Light_Attack2    = 2,
        Knight_Light_Attack3    = 3,
        Knight_Heavy_Attack     = 4,
        Knight_Special_Attack   = 5,
        Knight_Dodge            = 6,
        Knight_Parry            = 7,
        Knight_Stun             = 8,
        Knight_Hit              = 9,
        Knight_Guard            = 10,
        Knight_Drinking         = 11,
        Knight_Dead             = 12,

        /* Lancer [ 41 ~ 80 ] */

        /* Third Character [ 81 ~ 120 ] */

        /* Imp [ 121 ~ 160 ]  */

        /* Second Normal Monster [ 161 ~ 200 ] */

        /* Third Normal Monster [ 201 ~ 240 ] */

        /* First Boss Monster [ 241 ~ 280 ] */

        /* Second Boss Monster [ 281 ~ 320 ] */

        FinalBoss_Thrust = 321,
        FinalBoss_Slash = 322,
        FinalBoss_DashSlash = 323,
        FinalBoss_JumpSlash = 324,
        FinalBoss_MultiSlash = 325,
        FinalBoss_Stun = 326,
        FinalBoss_Hit = 327,
        FinalBoss_Dead = 328,
    }
    
    public enum ActionKind : byte
    {
        None        = 0,
        Attack      = 1,
        Dodge       = 2,
        Parry       = 3,
        Stun        = 4,
        Hit         = 5,
        Guard       = 6,
        UseItem     = 7,
        Dead        = 8,
    }

    public enum PlayerAttackInput : byte
    {
        None    = 0,
        Normal  = 1,
        Heavy   = 2,
        Special = 3,
    }

    /* [ Resource ID ] */
    public enum AnimationId : ushort
    {
        None                    = 0,

        Knight_Idle             = 1,
        Knight_Walk             = 2,
        Knight_Run              = 3,
        Knight_Attack           = 4,
        Knight_Dodge            = 5,
        Knight_Parry            = 6,
        Knight_Stun             = 7,
        Knight_Hit              = 8,
        Knight_Guard            = 9,
        Knight_Drinking         = 10,
        Knight_Dead             = 11,

        /* Lancer [ 41 ~ 80 ] */

        /* Third Character [ 81 ~ 120 ] */

        /* Imp [ 121 ~ 160 ]  */

        /* Second Normal Monster [ 161 ~ 200 ] */

        /* Third Normal Monster [ 201 ~ 240 ] */

        /* First Boss Monster [ 241 ~ 280 ] */

        /* Second Boss Monster [ 281 ~ 320 ] */

        FinalBoss_Idle          = 321,
        FinalBoss_Walk          = 322,
        FinalBoss_Thrust        = 323,
        FinalBoss_Slash         = 324,
        FinalBoss_DashSlash     = 325,
        FinalBoss_JumpSlash     = 326,
        FinalBoss_MultiSlash    = 327,
        FinalBoss_Stun          = 328,
        FinalBoss_Hit           = 329,
        FinalBoss_Dead          = 330,
    }


    /* [ World ID ] */
    public enum WorldId : byte
    {
        None                    = 0,
        Square                  = 1,
        Knight_Start            = 2,
        Lancer_Start            = 3,
        ThirdCharacter_Start    = 4,
        Middle                  = 5,
        Final                   = 6,
        Pvp                     = 7
    }

    public enum SpawnSetId : byte
    {
        None                        = 0,
        SquareDefault               = 1,
        KnightStartDefault          = 2,
        LancerStartDefault          = 3,
        ThirdCharacterStartDefault  = 4,
        MiddleDefault               = 5,
        FinalDefault                = 6,
        PvpDefault                  = 7,
    }
}
