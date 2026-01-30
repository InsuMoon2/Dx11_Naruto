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

HRESULT GameObject::Initialize(any arg)
{
    if (!arg.has_value())
        return S_OK;

    auto desc = any_cast<FGameObjectDesc>(arg);

    _transformCom = Transform::Create(_device, _context);

    if (FAILED(_transformCom->Initialize(desc)))
        return E_FAIL;

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

void GameObject::Render()
{

}

void GameObject::Free()
{
    Base::Free();

}
