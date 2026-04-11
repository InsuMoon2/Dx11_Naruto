#include "pch.h"
#include "Component.h"

Component::Component(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
    , _isCloned(false)
{
}

Component::Component(const Component& rhs)
    : _device(rhs._device), _context(rhs._context)
    , _isCloned(true)
{
}

Component::~Component()
{
}
       
HRESULT Component::Initialize(void* arg)
{

    return S_OK;
}

json Component::Reflect_ToJson() const
{
    json root = json::object();
    auto& info = const_cast<Component*>(this)->Get_ReflectionInfo();
    const char* basePtr = reinterpret_cast<const char*>(this);

    for (const auto& prop : info.properties)
    {
        const string key = prop.jsonKey.empty() ? prop.name : prop.jsonKey;
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
        {
            root[key] = *static_cast<const string*>(memberPtr);
            break;
        }
        }
    }

    return root;
}

void Component::Reflect_FromJson(const json& data)
{
    auto& info = Get_ReflectionInfo();
    char* basePtr = reinterpret_cast<char*>(this);

    for (const auto& prop : info.properties)
    {
        const string key = prop.jsonKey.empty() ? prop.name : prop.jsonKey;
        if (!data.contains(key)) continue;
        void* memberPtr = basePtr + prop.offset;

        switch (prop.type)
        {
        case EPropertyType::Float:
        case EPropertyType::ReadOnly:
            *static_cast<float*>(memberPtr) = data[key].get<float>();
            break;
        case EPropertyType::Int:
        case EPropertyType::Enum:
            *static_cast<int*>(memberPtr) = data[key].get<int>();
            break;
        case EPropertyType::Bool:
            *static_cast<bool*>(memberPtr) = data[key].get<bool>();
            break;
        case EPropertyType::Vec2:
        {
            auto arr = data[key];
            float* v = static_cast<float*>(memberPtr);
            v[0] = arr[0]; v[1] = arr[1];
            break;
        }
        case EPropertyType::Vec3:
        {
            auto arr = data[key];
            float* v = static_cast<float*>(memberPtr);
            v[0] = arr[0]; v[1] = arr[1]; v[2] = arr[2];
            break;
        }
        case EPropertyType::Vec4:
        case EPropertyType::Color:
        {
            auto arr = data[key];
            float* v = static_cast<float*>(memberPtr);
            v[0] = arr[0]; v[1] = arr[1]; v[2] = arr[2]; v[3] = arr[3];
            break;
        }
        case EPropertyType::String:
            *static_cast<string*>(memberPtr) = data[key].get<string>();
            break;
        }
    }
}

json Component::To_Json() const
{
    json j;

    auto id = static_cast<Protocol::ComponentID>(Get_ComponentID());
    j["type"] = string(magic_enum::enum_name(id));

    j.merge_patch(Reflect_ToJson());

    return j;
}

void Component::From_Json(const json& data)
{
    Reflect_FromJson(data);
}

HRESULT Component::Initialize_Prototype()
{

    return S_OK;
}

void Component::Free()
{
    Base::Free();

}
