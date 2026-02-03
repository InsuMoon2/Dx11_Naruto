#include "pch.h"
#include "Component_Factory.h"
#include "GameInstance.h"

// TODO : 이후 Reflection 진행하면 다시 추가, 기존 내용 Prototype Manager로 이동

map<uint32, wstring> Component_Factory::_prototypeMap;

void Component_Factory::Initialize()
{
    _prototypeMap.clear();
}

void Component_Factory::Register(uint32 typeId, const wstring& prototypeTag)
{
    if (_prototypeMap.contains(typeId))
    {
        MSG_BOX("Component Id Already Registered");
        return;
    }

    _prototypeMap.emplace(typeId, prototypeTag);
}

shared_ptr<Component> Component_Factory::Create(uint32 typeId, uint32 levelIndex, void* arg)
{
    if (!_prototypeMap.contains(typeId))
        return nullptr;

    const wstring& tag = _prototypeMap[typeId];

    //return GAME->Clone_Component(levelIndex, tag, arg);

    return nullptr;
}
