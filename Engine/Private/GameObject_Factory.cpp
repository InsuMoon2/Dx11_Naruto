#include "pch.h"
#include "GameObject_Factory.h"
#include "GameObject.h"

HRESULT GameObject_Factory::Initialize()
{

    return S_OK;
}

Shared<GameObject> GameObject_Factory::Create_Object(ComPtr<Device> device, ComPtr<DeviceContext> context,
    Protocol::OBJECT_TYPE type)
{
    auto& registry = Get_Registry();

    auto iter = registry.find(type);
    if (iter == registry.end())
        return nullptr;

    auto obj = iter->second.creator(device, context);

    if (obj)
        obj->Set_ObjectType(type);
    return obj;
}


shared_ptr<GameObject> GameObject_Factory::Create(const wstring& name, ComPtr<Device> device,
    ComPtr<DeviceContext> context)
{
    auto& registry = Get_Registry();

    for (const auto& [type, info] : registry)
    {
        if (info.name == name)
        {
            auto obj = info.creator(device, context);

            if (obj)
                obj->Set_ObjectType(type);

            return obj;
        }
    }

    return nullptr;
}

vector<wstring> GameObject_Factory::Get_RegisteredNames()
{
    auto& registry = Get_Registry();
    vector<wstring> names;

    names.reserve(registry.size());
    for (const auto& [type, info] : registry)
    {
        names.emplace_back(info.name);
    }
    return names;
}

Unique<GameObject_Factory> GameObject_Factory::Create()
{
    auto instance = make_unique<GameObject_Factory>();

    if (FAILED(instance->Initialize()))
    {
        LOG_WARN("게임오브젝트 팩토리 생성 실패");

        return nullptr;
    }
    return instance;
}

void GameObject_Factory::Free()
{
    Get_Registry().clear();

    Base::Free();
}



