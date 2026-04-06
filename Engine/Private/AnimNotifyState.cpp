#include "pch.h"
#include "AnimNotifyState.h"

static string Resolve_AnimNotifyStatePropertyJsonKey(const Engine::FPropertyInfo& prop)
{
    if (!prop.jsonKey.empty())
        return prop.jsonKey;

    return prop.name;
}

json AnimNotifyState::Serialize_Payload() const
{
    json root = json::object();

    auto& info = const_cast<AnimNotifyState*>(this)->Get_ReflectionInfo();
    const char* basePtr = reinterpret_cast<const char*>(this);

    for (const auto& prop : info.properties)
    {
        const string key = Resolve_AnimNotifyStatePropertyJsonKey(prop);
        const void* memberPtr = basePtr + prop.offset;

        switch (prop.type)
        {
        case EPropertyType::Float:
        case EPropertyType::ReadOnly:
            root[key] = *static_cast<const float*>(memberPtr);
            break;

        case EPropertyType::Int:
        case EPropertyType::Enum:
            root[key] = *static_cast<const int*>(memberPtr);
            break;

        case EPropertyType::Bool:
            root[key] = *static_cast<const bool*>(memberPtr);
            break;

        case EPropertyType::Vec2:
        {
            const float* v = static_cast<const float*>(memberPtr);
            root[key] = { v[0], v[1] };
            break;
        }

        case EPropertyType::Vec3:
        {
            const float* v = static_cast<const float*>(memberPtr);
            root[key] = { v[0], v[1], v[2] };
            break;
        }

        case EPropertyType::Vec4:
        case EPropertyType::Color:
        {
            const float* v = static_cast<const float*>(memberPtr);
            root[key] = { v[0], v[1], v[2], v[3] };
            break;
        }

        case EPropertyType::String:
            root[key] = *static_cast<const string*>(memberPtr);
            break;

        default:
            break;
        }
    }

    return root;
}

void AnimNotifyState::Deserialize_Payload(const json& payload)
{
    auto& info = Get_ReflectionInfo();
    char* basePtr = reinterpret_cast<char*>(this);

    for (const auto& prop : info.properties)
    {
        const string key = Resolve_AnimNotifyStatePropertyJsonKey(prop);
        if (!payload.contains(key))
            continue;

        void* memberPtr = basePtr + prop.offset;

        switch (prop.type)
        {
        case EPropertyType::Float:
        case EPropertyType::ReadOnly:
            *static_cast<float*>(memberPtr) = payload[key].get<float>();
            break;

        case EPropertyType::Int:
        case EPropertyType::Enum:
            *static_cast<int*>(memberPtr) = payload[key].get<int>();
            break;

        case EPropertyType::Bool:
            *static_cast<bool*>(memberPtr) = payload[key].get<bool>();
            break;

        case EPropertyType::Vec2:
        {
            auto arr = payload[key];
            float* v = static_cast<float*>(memberPtr);
            v[0] = arr[0];
            v[1] = arr[1];
            break;
        }

        case EPropertyType::Vec3:
        {
            auto arr = payload[key];
            float* v = static_cast<float*>(memberPtr);
            v[0] = arr[0];
            v[1] = arr[1];
            v[2] = arr[2];
            break;
        }

        case EPropertyType::Vec4:
        case EPropertyType::Color:
        {
            auto arr = payload[key];
            float* v = static_cast<float*>(memberPtr);
            v[0] = arr[0];
            v[1] = arr[1];
            v[2] = arr[2];
            v[3] = arr[3];
            break;
        }

        case EPropertyType::String:
            *static_cast<string*>(memberPtr) = payload[key].get<string>();
            break;

        default:
            break;
        }
    }
}
