#pragma once

NS_BEGIN(Engine)

enum class EPropertyType : uint8
{
    Bool,
    Int,
    Float,
    Vec2,
    Vec3,
    Vec4,
    Color,
    String,
    Enum,
    ReadOnly,  // 수정 불가. 근데 필요할지?
};

struct FPropertyInfo
{
    string          name;
    string          jsonKey;
    EPropertyType   type;
    size_t          offset;

    float           minVal = 0.f;
    float           maxVal = 0.f;
    float           dragSpeed = 0.1f;

    // Enum용
    vector<string>  enumNames;

};

struct FClassReflectionInfo
{
    string                  className;
    vector<FPropertyInfo>   properties;
};


NS_END
