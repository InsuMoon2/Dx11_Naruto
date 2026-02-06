#include "pch.h"
#include "GameObject_Factory.h"
#include "GameObject.h"

map<Protocol::OBJECT_TYPE, GameObject_Factory::FCreatorDesc> GameObject_Factory::_creators;

void GameObject_Factory::Initialize()
{
    //_creators.clear();

    int a = 10;
}

void GameObject_Factory::Register(OBJECT_TYPE type, const wstring& name, Creator creator)
{
    if (_creators.contains(type))
        return;

    _creators.emplace(type, FCreatorDesc{ name, creator });
}

shared_ptr<GameObject> GameObject_Factory::Create(OBJECT_TYPE type, ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto iter = _creators.find(type);
    if (iter == _creators.end())
        return nullptr;

    auto obj = iter->second.creator(device, context);
    if (obj)
        obj->Set_ObjectType(type);

    return obj;
}

shared_ptr<GameObject> GameObject_Factory::Create(const wstring& name, ComPtr<Device> device,
    ComPtr<DeviceContext> context)
{
    for (const auto& [type, info] : _creators)
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
    vector<wstring> names;
    names.reserve(_creators.size());

    for (const auto& [type, info] : _creators)
    {
        names.emplace_back(info.name);
    }

    return names;
}
