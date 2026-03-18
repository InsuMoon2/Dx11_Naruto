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

    enum class EPlayerState
    {
        Idle,
        Run,
        Jump,
        DoubleJump,

        SuperJumpCharge,
        SuperJump,
        HeightLand,

        //Attack,
        //Hit,
        Dash,
        //Skill,
        //
        //Dead,

        END
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
