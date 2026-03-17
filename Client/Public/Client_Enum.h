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
        SuperJump,

        //Attack,
        //Hit,
        //Dash,
        //Skill,
        //
        //Dead,

        END
    };

    // 애니메이션이 하나인지, Start -> Loop -> End로 세팅될건지
    enum class EStateAnimationMode { Single, Sequence, END };

}
