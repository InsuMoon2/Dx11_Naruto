#include "pch.h"
#include "Object_Manager.h"
#include "GameInstance.h"
#include "Layer.h"

Object_Manager::Object_Manager()
{
}

Object_Manager::~Object_Manager()
{
}

HRESULT Object_Manager::Initialize(uint32 numLevels)
{
    _numLevels = numLevels;

    _layers.resize(numLevels);

    return S_OK;
}

void Object_Manager::Priority_Update(float timeDelta)
{
    for (uint32 i = 0; i < _numLevels; i++)
    {
        for (auto& [tag, layer] : _layers[i])
        {
            if (layer)
                layer->Priority_Update(timeDelta);
        }
    }
}

void Object_Manager::Update(float timeDelta)
{
    for (uint32 i = 0; i < _numLevels; i++)
    {
        for (auto& [tag, layer] : _layers[i])
        {
            if (layer)
                layer->Update(timeDelta);
        }
    }
}

void Object_Manager::Late_Update(float timeDelta)
{
    for (uint32 i = 0; i < _numLevels; i++)
    {
        for (auto& [tag, layer] : _layers[i])
        {
            if (layer)
                layer->Late_Update(timeDelta);
        }
    }
}

HRESULT Object_Manager::Add_GameObject(uint32 protoLevelIndex, const wstring& protoTag, uint32 layerLevelIndex,
    const wstring& layerTag, any arg)
{
    if (layerLevelIndex >= _numLevels)
        return E_FAIL;

    auto gameObject = GAME->Clone_GameObject(protoLevelIndex, protoTag, arg);
    CHECK_NULL_RETURN(gameObject, E_FAIL);

    shared_ptr<Layer> layer = Find_Layer(layerLevelIndex, layerTag);

    // 레이어가 없다면, 생성 후 추가
    if (layer == nullptr)
    {
        layer = Layer::Create();
        _layers[layerLevelIndex].emplace(layerTag, layer);
    }

    // 레이어가 있으면, 추가
    layer->Add_GameObject(gameObject);

    return S_OK;
}

vector<shared_ptr<GameObject>> Object_Manager::Get_GameObjects(uint32 levelIndex)
{
    vector<shared_ptr<GameObject>> allObjects;

    if (levelIndex >= _numLevels)
        return allObjects;

    for (auto& [tag, layer] : _layers[levelIndex])
    {
        if (!layer)
            continue;

        const auto& layerObjects = layer->Get_GameObjects();

        for (auto& obj : layerObjects)
        {
            if (obj)
                allObjects.emplace_back(obj);
        }
    }

    return allObjects;
}

shared_ptr<Layer> Object_Manager::Find_Layer(uint32 levelIndex, const wstring& layerTag)
{
    auto iter = _layers[levelIndex].find(layerTag);

    if (iter == _layers[levelIndex].end())
        return nullptr;

    return iter->second;
}

unique_ptr<Object_Manager> Object_Manager::Create(uint32 numLevels)
{
    auto instance = make_unique<Object_Manager>();

    if (FAILED(instance->Initialize(numLevels)))
    {
        MSG_BOX("Failed to Create Object_Manager");

        return nullptr;
    }

    return instance;
}

void Object_Manager::Free()
{
    Base::Free();

    _layers.clear();
}
