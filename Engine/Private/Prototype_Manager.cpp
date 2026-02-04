#include "pch.h"
#include "Prototype_Manager.h"

#include "Component.h"
#include "GameObject.h"

Prototype_Manager::Prototype_Manager()
{
}

Prototype_Manager::~Prototype_Manager()
{
}

HRESULT Prototype_Manager::Initialize(uint32 numLevels)
{
    _numLevels = numLevels;

    _gameObjectPrototypes.resize(numLevels);
    _componentPrototypes.resize(numLevels);

    return S_OK;
}

HRESULT Prototype_Manager::Add_GameObject_Prototype(uint32 levelIndex, uint32 objID, shared_ptr<GameObject> prototype)
{
    if (Find_GameObject_Prototype(levelIndex, objID) != nullptr)
    {
        MSG_BOX("This object is already exists.");

        return E_FAIL;
    }

    _gameObjectPrototypes[levelIndex].emplace(objID, prototype);

    return S_OK;
}

shared_ptr<GameObject> Prototype_Manager::Clone_GameObject(uint32 levelIndex, uint32 objID, void* arg)
{
    auto gameObject = Find_GameObject_Prototype(levelIndex, objID);
    CHECK_NULL(gameObject, nullptr);

    return gameObject->Clone(arg);
}

HRESULT Prototype_Manager::Add_Component_Prototype(uint32 levelIndex, uint32 componentID, shared_ptr<Component> prototype)
{
    if (Find_Component_Prototype(levelIndex, componentID) != nullptr)
    {
        MSG_BOX("This component is already exists.");

        return E_FAIL;
    }

    _componentPrototypes[levelIndex].emplace(componentID, prototype);

    return S_OK;
}

shared_ptr<Component> Prototype_Manager::Clone_Component(uint32 levelIndex, uint32 componentID, void* arg)
{
    auto component = Find_Component_Prototype(levelIndex, componentID);
    CHECK_NULL(component, nullptr);

    return component->Clone(arg);
}

HRESULT Prototype_Manager::Clear_Prototype(uint32 levelIndex)
{
    if (levelIndex >= _numLevels)
        return E_FAIL;

    _gameObjectPrototypes[levelIndex].clear();
    _componentPrototypes[levelIndex].clear();

    return S_OK;
}

shared_ptr<GameObject> Prototype_Manager::Find_GameObject_Prototype(uint32 levelIndex, uint32 objID)
{
    if (levelIndex >= _numLevels)
        return nullptr;

    auto iter = _gameObjectPrototypes[levelIndex].find(objID);

    if (iter == _gameObjectPrototypes[levelIndex].end())
        return nullptr;

    return iter->second;
}

shared_ptr<Component> Prototype_Manager::Find_Component_Prototype(uint32 levelIndex, uint32 componentID)
{
    if (levelIndex >= _numLevels)
        return nullptr;

    auto iter = _componentPrototypes[levelIndex].find(componentID);

    if (iter == _componentPrototypes[levelIndex].end())
        return nullptr;

    return iter->second;
}

unique_ptr<Prototype_Manager> Prototype_Manager::Create(uint32 numLevels)
{
    auto instance = make_unique<Prototype_Manager>();

    if (FAILED(instance->Initialize(numLevels)))
    {
        MSG_BOX("Failed to Created : Prototype Manager");

        return nullptr;
    }

    return instance;
}

void Prototype_Manager::Free()
{
    Base::Free();

    for (uint32 i = 0; i < _numLevels; ++i)
    {
        _gameObjectPrototypes[i].clear();
        _componentPrototypes[i].clear();
    }

    _gameObjectPrototypes.clear();
    _componentPrototypes.clear();
}
