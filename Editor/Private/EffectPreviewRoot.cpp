#include "pch.h"
#include "EffectPreviewRoot.h"

EffectPreviewRoot::EffectPreviewRoot(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

EffectPreviewRoot::EffectPreviewRoot(const EffectPreviewRoot& rhs)
    : GameObject(rhs)
{
}

HRESULT EffectPreviewRoot::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT EffectPreviewRoot::Initialize(void* arg)
{
    return GameObject::Initialize(arg);
}

void EffectPreviewRoot::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);

}
void EffectPreviewRoot::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

}
void EffectPreviewRoot::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

}
HRESULT EffectPreviewRoot::Render()
{
    return S_OK;
} 

Shared<GameObject> EffectPreviewRoot::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<EffectPreviewRoot>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : EffectPreviewRoot");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> EffectPreviewRoot::Clone(void* arg)
{
    auto clone = make_shared<EffectPreviewRoot>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : EffectPreviewRoot");
        return nullptr;
    }

    return clone;
}

void EffectPreviewRoot::Free()
{
    GameObject::Free();
}
