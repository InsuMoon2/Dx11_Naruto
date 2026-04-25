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
        //MSG_BOX("This object is already exists.");

        //return E_FAIL;
    }

    _gameObjectPrototypes[levelIndex].emplace(objID, prototype);

    return S_OK;
}

shared_ptr<GameObject> Prototype_Manager::Clone_GameObject(uint32 levelIndex, uint32 objID, void* arg)
{
    auto gameObject = Find_GameObject_Prototype(levelIndex, objID);

    if (gameObject == nullptr && levelIndex != 0)
    {
        gameObject = Find_GameObject_Prototype(0, objID);
    }

    CHECK_NULL(gameObject, nullptr);

    return gameObject->Clone(arg);
}

HRESULT Prototype_Manager::Add_Component_Prototype(uint32 levelIndex, uint32 componentID, shared_ptr<Component> prototype)
{
    if (Find_Component_Prototype(levelIndex, componentID) != nullptr)
    {
#pragma region Return
        {
            //MSG_BOX("This component is already exists.");
            //return E_FAIL;
        }
#pragma endregion


#pragma region Skip
        {
            LOG_WARN("Exist Component ID : {} ", componentID);
            return S_OK;
        }
#pragma endregion
            

    }

    _componentPrototypes[levelIndex].emplace(componentID, prototype);

    return S_OK;
}

shared_ptr<Component> Prototype_Manager::Clone_Component(uint32 levelIndex, uint32 componentID, void* arg)
{
    auto component = Find_Component_Prototype(levelIndex, componentID);

    if (!component)
    {
        LOG_ERROR("Clone_Component failed. levelIndex={}, componentID={}", levelIndex, componentID);
        return nullptr;
    }

    return component->Clone(arg);
}

shared_ptr<Component> Prototype_Manager::Clone_Component(uint32 componentID, void* arg)
{
    // 전역으로 우선 탐색 후, 없다면 최근 레벨에서 탐색
    auto component = Find_Component_Prototype(0, componentID);

    if (!component)
        component = Find_Component_Prototype(GAME->Current_Level(), componentID);

    if (!component)
    {
        LOG_ERROR("Clone_Component failed. searched Static and current level. currentLevel={}, componentID={}",
            GAME->Current_Level(),
            componentID);

        return nullptr;
    }

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

vector<pair<uint32, wstring>> Prototype_Manager::Get_RegisteredGameObjects()
{
    vector<pair<uint32, wstring>> result;

    // Static 레벨에서 가져오기
    for (const auto& [objID, proto] : _gameObjectPrototypes[0])
    {
        result.emplace_back(objID, proto->Get_Name());
    }

    return result;
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
