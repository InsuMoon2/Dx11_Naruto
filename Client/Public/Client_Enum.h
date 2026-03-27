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
        Idle,
        Run,
        Jump,
        DoubleJump,
        JumpDash,

        SuperJumpCharge,
        SuperJump,
        HeightLand,

        Attack,
        Hit,
        Dash,


        // 스킬 string으로 가능하긴 한데, 안전하게 Enum처리하기
        Skill_Rasengan,
        Skill_RasenShuriken,

        Dead,
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

    enum class EMoveInputDirection { Forward, Backward, Left, Right, END };

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
