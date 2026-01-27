#include "pch.h"
#include "GameObject.h"

GameObject::GameObject(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{
}

GameObject::GameObject(const GameObject& rhs)
    : _device(rhs._device), _context(rhs._context)
{
}

GameObject::~GameObject()
{
}

HRESULT GameObject::Initialize(void* arg)
{

    return S_OK;
}

HRESULT GameObject::Initialize_Prototype()
{

    return S_OK;
}

void GameObject::Priority_Update(float timeDelta)
{

}

void GameObject::Update(float timeDelta)
{

}

void GameObject::Late_Update(float timeDelta)
{

}

void GameObject::Free()
{
    Base::Free();

}
