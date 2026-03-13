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



}
