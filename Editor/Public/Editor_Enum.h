#pragma once

NS_BEGIN(Editor)

enum class LogLevel { Info, Warning, Error, END };

enum class ENotifyType { Info, Success, Warning, Error };

enum class EAssetOpenType
{
    None,
    Prefab,
    BehaviorTree,
    UIAnimation,
    Effect,
    Mesh,
    Texture,
    Csv,

    END
};

enum class EAnimNotifyTrackType : uint8
{
    Notify,
    NotifyState,

    END
};

NS_END
