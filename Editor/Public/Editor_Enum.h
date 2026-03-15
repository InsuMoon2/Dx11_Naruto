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
    Mesh,
    Texture,
    Csv,

    END
};

NS_END
