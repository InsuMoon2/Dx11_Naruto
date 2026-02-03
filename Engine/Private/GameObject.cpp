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
    FGameObjectDesc* desc = static_cast<FGameObjectDesc*>(arg);

    _transformCom = Transform::Create(_device, _context);
    CHECK_NULL(_transformCom, E_FAIL);

    CHECK_FAILED(_transformCom->Initialize(desc), E_FAIL);

    _components.emplace(Transform::GetComponentID(), _transformCom);

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

    _components.clear();
}
