#pragma once


#pragma warning(push)
#pragma warning(disable: 4648)

// ==================================================
//              프로퍼티 등록
// ==================================================

#define REFLECT_BEGIN(ClassName)                                          \
    static bool _s_##ClassName##_reflRegistered = []() {                 \
        using SelfType = ClassName;                                      \
        auto& info = ClassName::GetStaticReflectionInfo();               \
        info.className = #ClassName;

#define REFLECT_END                                                      \
        return true;                                                     \
    }();


#define PROPERTY_FLOAT(DisplayName, Member, Min, Max)                      \
    info.properties.push_back({                                             \
        DisplayName, Engine::EPropertyType::Float,                          \
        offsetof(SelfType, Member), Min, Max, 0.1f, {}                      \
    });

#define PROPERTY_VEC3(DisplayName, Member, Speed)                          \
    info.properties.push_back({                                             \
        DisplayName, Engine::EPropertyType::Vec3,                           \
        offsetof(SelfType, Member), 0.f, 0.f, Speed, {}                     \
    });

#define PROPERTY_BOOL(DisplayName, Member)                                 \
    info.properties.push_back({                                             \
        DisplayName, Engine::EPropertyType::Bool,                           \
        offsetof(SelfType, Member), 0.f, 0.f, 0.f, {}                       \
    });

#define PROPERTY_INT(DisplayName, Member, Min, Max)                        \
    info.properties.push_back({                                             \
        DisplayName, Engine::EPropertyType::Int,                            \
        offsetof(SelfType, Member), (float)Min, (float)Max, 1.f, {}         \
    });

#define PROPERTY_COLOR(DisplayName, Member)                                \
    info.properties.push_back({                                             \
        DisplayName, Engine::EPropertyType::Color,                          \
        offsetof(SelfType, Member), 0.f, 1.f, 0.01f, {}                     \
    });

#define PROPERTY_READONLY(DisplayName, Member)                             \
    info.properties.push_back({                                             \
        DisplayName, Engine::EPropertyType::ReadOnly,                       \
        offsetof(SelfType, Member), 0.f, 0.f, 0.f, {}                       \
    });

#define PROPERTY_ENUM(DisplayName, Member, EnumType)                       \
{                                                                            \
    Engine::FPropertyInfo _prop;                                             \
    _prop.name   = DisplayName;                                              \
    _prop.type   = Engine::EPropertyType::Enum;                              \
    _prop.offset = offsetof(SelfType, Member);                               \
    auto _names  = magic_enum::enum_names<EnumType>();                       \
    for (auto& _n : _names) _prop.enumNames.push_back(string(_n));           \
    info.properties.push_back(_prop);                                        \
}

#define PROPERTY_STRING(DisplayName, Member)                               \
    info.properties.push_back({                                             \
        DisplayName, Engine::EPropertyType::String,                         \
        offsetof(SelfType, Member), 0.f, 0.f, 0.f, {}                       \
    });

#define PROPERTY_ENUM_CUSTOM(DisplayName, Member, EnumNamesVec)            \
{                                                                            \
    Engine::FPropertyInfo _prop;                                             \
    _prop.name   = DisplayName;                                              \
    _prop.type   = Engine::EPropertyType::Enum;                              \
    _prop.offset = offsetof(SelfType, Member);                               \
    _prop.enumNames = EnumNamesVec;                                          \
    info.properties.push_back(_prop);                                        \
}

#pragma warning(pop)
