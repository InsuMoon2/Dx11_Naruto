#pragma once

namespace Client
{
    enum class ELoadJobType
    {
        Shader,
        TextureCreate,
        TextureAppend,
        Terrain,
        Model,
        Skill,
        GameObjectPrototype,
        LevelChunk,

        END
    };

    enum class EGameplaySpawnMode { LocalOnly, Server, END };

    enum class EPlayerState
    {
        Idle,       BigSword_Idle,  Wall_Idle,
        Run,        Wall_Run,
        Jump,
        JumpFall,
        DoubleJump,
        JumpDash,

        SuperJumpCharge,
        SuperJump,
        HeightLand,

        WireDash,
        AirApproach,

        // Attack 진입 판별용
        Attack, JumpAttack,

        // Attack에 진입하면, ComboProfile에 따라 공격 상태값 알아서 세팅
        Attack_01,              Attack_02,              Attack_03,          Attack_04,
        Attack_Air_01,          Attack_Air_02,          Attack_Air_03,      Attack_Air_04,
        Attack_Sword_01,        Attack_Sword_02,        Attack_Sword_03,    Attack_Sword_04,
        Attack_SwordAir_01,     Attack_SwordAir_02,

        Hit,
        Dash,

        // 스킬 string으로 가능하긴 한데, 안전하게 Enum처리하기
        Skill_Rasengan,         Skill_Rasengan_Air,     Skill_Rasengan_End, 
        Skill_RasenShuriken,    Skill_RasenShuriken_Air,

        Skill_Chidori,          Skill_Chidori_Air,      Skill_Chidori_End,
        Skill_BlastBoomDance,   Skill_BlastBoomDance_Air,

        // 스킬 두개 정도만 더 추가할까

        Dead,
        END
    };

    enum class EAttackProfileType
    {
        Hand_Ground,
        Hand_Aerial,

        BigSword_Ground,
        BigSword_Aerial,

        END
    };

    enum class EMonsterState
    {
        Idle,
        Run,
        Attack,
        Hit,
        Dead,
        END
    };

    enum class EWeaponType
    {
        Hand,
        BigSwrod,
    };

    enum class EHitboxTarget : uint8
    {
        RightHand,
        LeftHand,
        RightFoot,
        LeftFoot,

        END
    };

    // 애니메이션이 하나인지, Start -> Loop -> End로 세팅될건지
    enum class EStateAnimationMode { Single, Sequence, DirectionalSingle, END };

    enum class EPlayerInputMode
    {
        Normal,         // 전부 키기
        LookOnly,       // 시점 이동만 허용
        MoveAndLook,    // 이동 + 시점
        BlockAll,       // 전부 차단

        END
    };

    
}
