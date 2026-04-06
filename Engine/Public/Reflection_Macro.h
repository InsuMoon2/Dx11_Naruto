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
        DisplayName, "", Engine::EPropertyType::Float,                      \
        offsetof(SelfType, Member), Min, Max, 0.1f, {}                      \
    });

#define PROPERTY_VEC2(DisplayName, Member, Speed)                          \
    info.properties.push_back({                                             \
        DisplayName, "", Engine::EPropertyType::Vec2,                       \
        offsetof(SelfType, Member), 0.f, 0.f, Speed, {}                     \
    });

#define PROPERTY_VEC3(DisplayName, Member, Speed)                          \
    info.properties.push_back({                                             \
        DisplayName, "", Engine::EPropertyType::Vec3,                       \
        offsetof(SelfType, Member), 0.f, 0.f, Speed, {}                     \
    });

#define PROPERTY_VEC4(DisplayName, Member, Speed)                          \
    info.properties.push_back({                                             \
        DisplayName, "", Engine::EPropertyType::Vec4,                       \
        offsetof(SelfType, Member), 0.f, 0.f, Speed, {}                     \
    });

#define PROPERTY_BOOL(DisplayName, Member)                                 \
    info.properties.push_back({                                             \
        DisplayName, "", Engine::EPropertyType::Bool,                       \
        offsetof(SelfType, Member), 0.f, 0.f, 0.f, {}                       \
    });

#define PROPERTY_INT(DisplayName, Member, Min, Max)                        \
    info.properties.push_back({                                             \
        DisplayName, "", Engine::EPropertyType::Int,                        \
        offsetof(SelfType, Member), (float)Min, (float)Max, 1.f, {}         \
    });

#define PROPERTY_COLOR(DisplayName, Member)                                \
    info.properties.push_back({                                             \
        DisplayName, "", Engine::EPropertyType::Color,                      \
        offsetof(SelfType, Member), 0.f, 1.f, 0.01f, {}                     \
    });

#define PROPERTY_READONLY(DisplayName, Member)                             \
    info.properties.push_back({                                             \
        DisplayName, "", Engine::EPropertyType::ReadOnly,                   \
        offsetof(SelfType, Member), 0.f, 0.f, 0.f, {}                       \
    });

#define PROPERTY_ENUM(DisplayName, Member, EnumType)                       \
{                                                                            \
    Engine::FPropertyInfo _prop;                                             \
    _prop.name   = DisplayName;                                              \
    _prop.jsonKey = "";                                                     \
    _prop.type   = Engine::EPropertyType::Enum;                              \
    _prop.offset = offsetof(SelfType, Member);                               \
    auto _names  = magic_enum::enum_names<EnumType>();                       \
    for (auto& _n : _names) _prop.enumNames.push_back(string(_n));           \
    info.properties.push_back(_prop);                                        \
}

#define PROPERTY_STRING(DisplayName, Member)                               \
    info.properties.push_back({                                             \
        DisplayName, "", Engine::EPropertyType::String,                     \
        offsetof(SelfType, Member), 0.f, 0.f, 0.f, {}                       \
    });

#define PROPERTY_ENUM_CUSTOM(DisplayName, Member, EnumNamesVec)            \
{                                                                            \
    Engine::FPropertyInfo _prop;                                             \
    _prop.name   = DisplayName;                                              \
    _prop.jsonKey = "";                                                     \
    _prop.type   = Engine::EPropertyType::Enum;                              \
    _prop.offset = offsetof(SelfType, Member);                               \
    _prop.enumNames = EnumNamesVec;                                          \
    info.properties.push_back(_prop);                                        \
}

#define PROPERTY_FLOAT_JSON(DisplayName, JsonKey, Member, Min, Max)        \
    info.properties.push_back({                                             \
        DisplayName, JsonKey, Engine::EPropertyType::Float,                 \
        offsetof(SelfType, Member), Min, Max, 0.1f, {}                      \
    });

#define PROPERTY_VEC2_JSON(DisplayName, JsonKey, Member, Speed)            \
    info.properties.push_back({                                             \
        DisplayName, JsonKey, Engine::EPropertyType::Vec2,                  \
        offsetof(SelfType, Member), 0.f, 0.f, Speed, {}                     \
    });

#define PROPERTY_VEC3_JSON(DisplayName, JsonKey, Member, Speed)            \
    info.properties.push_back({                                             \
        DisplayName, JsonKey, Engine::EPropertyType::Vec3,                  \
        offsetof(SelfType, Member), 0.f, 0.f, Speed, {}                     \
    });

#define PROPERTY_VEC4_JSON(DisplayName, JsonKey, Member, Speed)            \
    info.properties.push_back({                                             \
        DisplayName, JsonKey, Engine::EPropertyType::Vec4,                  \
        offsetof(SelfType, Member), 0.f, 0.f, Speed, {}                     \
    });

#define PROPERTY_BOOL_JSON(DisplayName, JsonKey, Member)                   \
    info.properties.push_back({                                             \
        DisplayName, JsonKey, Engine::EPropertyType::Bool,                  \
        offsetof(SelfType, Member), 0.f, 0.f, 0.f, {}                       \
    });

#define PROPERTY_INT_JSON(DisplayName, JsonKey, Member, Min, Max)          \
    info.properties.push_back({                                             \
        DisplayName, JsonKey, Engine::EPropertyType::Int,                   \
        offsetof(SelfType, Member), (float)Min, (float)Max, 1.f, {}         \
    });

#define PROPERTY_COLOR_JSON(DisplayName, JsonKey, Member)                  \
    info.properties.push_back({                                             \
        DisplayName, JsonKey, Engine::EPropertyType::Color,                 \
        offsetof(SelfType, Member), 0.f, 1.f, 0.01f, {}                     \
    });

#define PROPERTY_READONLY_JSON(DisplayName, JsonKey, Member)               \
    info.properties.push_back({                                             \
        DisplayName, JsonKey, Engine::EPropertyType::ReadOnly,              \
        offsetof(SelfType, Member), 0.f, 0.f, 0.f, {}                       \
    });

#define PROPERTY_STRING_JSON(DisplayName, JsonKey, Member)                 \
    info.properties.push_back({                                             \
        DisplayName, JsonKey, Engine::EPropertyType::String,                \
        offsetof(SelfType, Member), 0.f, 0.f, 0.f, {}                       \
    });

#define PROPERTY_ENUM_JSON(DisplayName, JsonKey, Member, EnumType)         \
{                                                                            \
    Engine::FPropertyInfo _prop;                                             \
    _prop.name = DisplayName;                                                \
    _prop.jsonKey = JsonKey;                                                 \
    _prop.type = Engine::EPropertyType::Enum;                                \
    _prop.offset = offsetof(SelfType, Member);                               \
    auto _entries = magic_enum::enum_entries<EnumType>();                    \
    for (auto& _e : _entries) {                                              \
        _prop.enumNames.push_back(string(_e.second));                        \
        _prop.enumValues.push_back(static_cast<int>(_e.first));              \
    }                                                                        \
    info.properties.push_back(_prop);                                        \
}

#define PROPERTY_ENUM_CUSTOM_JSON(DisplayName, JsonKey, Member, EnumNamesVec) \
{                                                                               \
    Engine::FPropertyInfo _prop;                                                \
    _prop.name = DisplayName;                                                   \
    _prop.jsonKey = JsonKey;                                                    \
    _prop.type = Engine::EPropertyType::Enum;                                   \
    _prop.offset = offsetof(SelfType, Member);                                  \
    _prop.enumNames = EnumNamesVec;                                             \
    info.properties.push_back(_prop);                                           \
}

#pragma warning(pop)
