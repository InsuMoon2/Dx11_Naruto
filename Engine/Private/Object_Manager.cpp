#include "pch.h"
#include "Object_Manager.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "Layer.h"
#include "Event_Manager.h"

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

    // 이벤트
    Bind_Events();

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

void Object_Manager::Clear_Layers(uint32 levelIndex)
{
    if (levelIndex >= _numLevels)
        return;

    _layers[levelIndex].clear();
}

HRESULT Object_Manager::Add_GameObject(uint32 protoLevelIndex, uint32 objID, uint32 layerLevelIndex,
                                       const wstring& layerTag, void* arg)
{
    if (layerLevelIndex >= _numLevels)
        return E_FAIL;

    auto gameObject = GAME->Clone_GameObject(protoLevelIndex, objID, arg);
    CHECK_NULL(gameObject, E_FAIL);

    shared_ptr<Layer> layer = Find_Layer(layerLevelIndex, layerTag);

    // 레이어가 없다면, 생성 후 추가
    if (layer == nullptr)
    {
        layer = Layer::Create();
        _layers[layerLevelIndex].emplace(layerTag, layer);
    }

    // 레이어가 있으면, 추가
    layer->Add_GameObject(gameObject);
    gameObject->BeginPlay();

    return S_OK;
}

Shared<GameObject> Object_Manager::Clone_And_Add_GameObject(uint32 protoIndex, uint32 objID, uint32 levelIndex,
    const wstring& layerTag, void* arg)
{
    if (levelIndex >= _numLevels)
        return nullptr;

    auto gameObject = GAME->Clone_GameObject(protoIndex, objID, arg);

    if (!gameObject)
        return nullptr;

    shared_ptr<Layer> layer = Find_Layer(levelIndex, layerTag);

    if (layer == nullptr)
    {
        layer = Layer::Create();
        _layers[levelIndex].emplace(layerTag, layer);
    }

    layer->Add_GameObject(gameObject);

    gameObject->BeginPlay();

    return gameObject;    
}

HRESULT Object_Manager::Add_GameObject(uint32 levelIndex, const wstring& layerTag, shared_ptr<GameObject> gameObject)
{
    if (levelIndex >= _numLevels || gameObject == nullptr)
        return E_FAIL;

    shared_ptr<Layer> layer = Find_Layer(levelIndex, layerTag);

    if (layer == nullptr)
    {
        layer = Layer::Create();

        if (layer == nullptr)
            return E_FAIL;

        _layers[levelIndex].emplace(layerTag, layer);
    }

    HRESULT hr = layer->Add_GameObject(gameObject);

    if (SUCCEEDED(hr))
        gameObject->BeginPlay();

    return hr;
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

const umap<wstring, shared_ptr<Layer>>& Object_Manager::Get_Layers(uint32 levelIndex)
{
    static umap<wstring, shared_ptr<Layer>> emptyLayers;

    if (levelIndex >= _numLevels)
        return emptyLayers;

    return _layers[levelIndex];
}

void Object_Manager::Bind_Events()
{
    // 삭제
    EVENT->Subscribe(EEventType::Delete_Object, [this](shared_ptr<FEvent> event)
        {
            this->OnDeleteEvent(event);
        });

    // 생성
    EVENT->Subscribe(EEventType::Create_Object, [this](shared_ptr<FEvent> event)
        {
            this->OnCreateEvent(event);
        });
}

shared_ptr<Layer> Object_Manager::Find_Layer(uint32 levelIndex, const wstring& layerTag)
{
    auto iter = _layers[levelIndex].find(layerTag);

    if (iter == _layers[levelIndex].end())
        return nullptr;

    return iter->second;
}

void Object_Manager::OnDeleteEvent(shared_ptr<FEvent> event)
{
    auto deleteEvent = static_pointer_cast<FEvent_Object>(event);

    if (deleteEvent && deleteEvent->targetObject)
    {
        this->Delete_GameObject(GAME->Current_Level(), deleteEvent->targetObject);
    }
}

void Object_Manager::OnCreateEvent(shared_ptr<FEvent> event)
{
    auto createEvent = static_pointer_cast<FEvent_Object>(event);
    auto& targetObj = createEvent->targetObject;

    if (targetObj)
    {
        uint32 currentLevelIndex = GAME->Current_Level();

        // 레이어찾기
        shared_ptr<Layer> targetLayer = {};

        for (auto& [layerTag, layer] : _layers[currentLevelIndex])
        {
            if (!layer)
                continue;

            auto& objects = layer->Get_GameObjects();

            auto iter = find(objects.begin(), objects.end(), targetObj);

            if (iter != objects.end())
            {
                targetLayer = layer;
                break;
            }
        }

        auto newObj = targetObj->Clone(nullptr);

        auto srcT = targetObj->Get_Component<Transform>();
        auto dstT = newObj->Get_Component<Transform>();

        if (srcT && dstT)
        {
            dstT->Set_LocalPosition(srcT->Get_LocalPosition());
            dstT->Set_LocalRotation(srcT->Get_LocalRotation());
            dstT->Set_LocalScale(srcT->Get_LocalScale());
        }

        if (targetLayer)
        {
            targetLayer->Add_GameObject(newObj);

            // 이름 변경 어떻게 할지 ?
            newObj->Set_Name(targetObj->Get_Name() + L"_Copy");

            newObj->BeginPlay();
        }
    }
}

void Object_Manager::Delete_GameObject(uint32 levelIndex, shared_ptr<GameObject> gameObject)
{
    if (levelIndex >= _numLevels || gameObject == nullptr)
        return;

    for (auto& pair : _layers[levelIndex])
    {
        pair.second->Delete_GameObject(gameObject);
    }
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
