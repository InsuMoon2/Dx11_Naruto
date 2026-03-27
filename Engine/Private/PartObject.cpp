#include "pch.h"
#include "PartObject.h"

PartObject::PartObject(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_PART_OBJECT);
}

PartObject::PartObject(const PartObject& rhs)
    : GameObject(rhs)
{
}

HRESULT PartObject::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT PartObject::Initialize(void* arg)
{
    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    FPartObjectDesc* partDesc = static_cast<FPartObjectDesc*>(arg);
    if (partDesc)
    {
        //_parentMatrix = partDesc->parentMatrix;

        _parentTransform = partDesc->parentTransform;
    }

    return S_OK;
}

void PartObject::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);

}

void PartObject::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

}

void PartObject::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

}

HRESULT PartObject::Render()
{
    CHECK_FAILED(GameObject::Render(), E_FAIL);

    return S_OK;
}

void PartObject::Free()
{
    GameObject::Free();
}
