#include "pch.h"
#include "Component_Factory.h"
#include "Component.h"
#include "GameInstance.h"

IMPLEMENT_SINGLETON(Component_Factory)

void Component_Factory::Initialize()
{
    _creators.clear();
    _prototypeMap.clear();
    _classNames.clear();
}

void Component_Factory::Register(uint32 typeId, Creator creator, const wstring& className)
{
    if (_creators.contains(typeId))
        return;

    _creators.emplace(typeId, creator);
    _classNames.emplace(typeId, className);
}

shared_ptr<Component> Component_Factory::Create(uint32 typeId, ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto iter = _creators.find(typeId);
    if (iter == _creators.end())
        return nullptr;

    return iter->second(device, context);
}

void Component_Factory::Register_Prototype(uint32 typeId, const wstring& prototypeTag)
{
    if (_prototypeMap.contains(typeId))
    {
        MSG_BOX("Component Id Already Registered");
        return;
    }
    _prototypeMap.emplace(typeId, prototypeTag);
}

shared_ptr<Component> Component_Factory::Clone_Prototype(uint32 typeId, uint32 levelIndex, void* arg)
{
    if (!_prototypeMap.contains(typeId))
        return nullptr;

    return GAME->Clone_Component(levelIndex, typeId, arg);
}

vector<uint32> Component_Factory::Get_RegisteredComponentIds()
{
    vector<uint32> ids;
    ids.reserve(_creators.size());

    for (const auto& pair : _creators)
    {
        ids.push_back(pair.first);
    }

    return ids;
}

vector<pair<uint32, wstring>> Component_Factory::Get_RegisteredComponents()
{
    vector<pair<uint32, wstring>> components;
    components.reserve(_creators.size());

    for (const auto& pair : _classNames)
    {
        components.push_back(pair); 
    }
    return components;
}
