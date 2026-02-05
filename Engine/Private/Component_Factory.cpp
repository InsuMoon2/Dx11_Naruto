#include "pch.h"
#include "Component_Factory.h"
#include "Component.h"
#include "GameInstance.h"

map<uint32, Component_Factory::Creator> Component_Factory::_creators;
map<uint32, wstring> Component_Factory::_prototypeMap;

void Component_Factory::Initialize()
{
    _creators.clear();
    _prototypeMap.clear();
}

void Component_Factory::Register(uint32 typeId, Creator creator)
{
    if (_creators.contains(typeId))
        return;
    _creators.emplace(typeId, creator);
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
