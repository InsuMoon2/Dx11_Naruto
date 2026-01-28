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
    NULL_CHECK_RETURN(gameObject, E_FAIL);

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
}

shared_ptr<Layer> Layer::Create()
{
    return make_shared<Layer>();
}

void Layer::Free()
{
    Base::Free();
}
