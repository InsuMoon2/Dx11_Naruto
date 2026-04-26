#include "pch.h"
#include "Layer.h"

#include "GameObject.h"

Layer::Layer()
{
}

Layer::~Layer()
{
}

HRESULT Layer::Add_GameObject(shared_ptr<GameObject> gameObject)
{
    CHECK_NULL(gameObject, E_FAIL);

    _gameObjects.emplace_back(gameObject);

    return S_OK;
}

void Layer::Priority_Update(float timeDelta)
{
    for (auto& gameObject : _gameObjects)
    {
        gameObject->Priority_Update(timeDelta);
    }
}

void Layer::Update(float timeDelta)
{
    for (auto& gameObject : _gameObjects)
    {
        gameObject->Update(timeDelta);
    }
}

void Layer::Late_Update(float timeDelta)
{
    for (auto& gameObject : _gameObjects)
    {
        gameObject->Late_Update(timeDelta);
    }

    Cleanup_DestroyedObjects();
}

void Layer::Delete_GameObject(shared_ptr<GameObject> gameObject)
{
    auto iter = find(_gameObjects.begin(), _gameObjects.end(), gameObject);

    if (iter != _gameObjects.end())
    {
        if (*iter)
            (*iter)->On_RemovedFromLevel();

        _gameObjects.erase(iter);
    }
        
}

void Layer::Cleanup_DestroyedObjects()
{
    _gameObjects.erase(
        remove_if(_gameObjects.begin(), _gameObjects.end(),
            [](const shared_ptr<GameObject>& gameObject)
            {
                return !gameObject || gameObject->Is_Destroy();
            }),
        _gameObjects.end());
}

shared_ptr<Layer> Layer::Create()
{
    return make_shared<Layer>();
}

void Layer::Free()
{
    Base::Free();

    _gameObjects.clear();
}
