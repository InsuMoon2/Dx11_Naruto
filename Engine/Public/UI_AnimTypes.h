#pragma once

NS_BEGIN(Engine)

enum class EUIAnimProperty
{
    PositionX,
    PositionY,
    RotationZ,
    ScaleX,
    ScaleY,
    Alpha,
    ColorRGBA,
    END
};

enum class EUIAnimInterplation
{
    Linear,

    // 더 추가 ? 할지

    END
};

struct FUIAnimKey
{
    int32               frame = 0;

    // 스칼라 트랙은 x만 사용
    // ColorRGBA는 xyzw값 모두 사용
    Vec4                value = Vec4::Zero;
    EUIAnimInterplation interpolation = EUIAnimInterplation::Linear;
};

struct FUIAnimTrack
{
    EUIAnimProperty     property = EUIAnimProperty::PositionX;
    vector<FUIAnimKey>  keys;
};

struct FUIAnimAsset
{
    string  name;
    wstring targetName;

    int32   fps = 60;
    int32   startFrame = 0;
    int32   endFrame = 0;
    bool    loop = false;

    vector<FUIAnimTrack> tracks;
};

NS_END
