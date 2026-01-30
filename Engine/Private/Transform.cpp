#include "pch.h"
#include "Transform.h"


Transform::Transform(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
    
}

Transform::Transform(const Transform& protoType) : Component(protoType)
{
    
}

Transform::~Transform()
{
    
}

HRESULT Transform::Initialize_Prototype()
{
    return S_OK;
}

HRESULT Transform::Initialize(any arg)
{
    if (!arg.has_value())
        return S_OK;

    auto desc = any_cast<FTransformDesc>(arg);

    _speedPerSec = desc.speedPerSec;
    _rotationPerSec = desc.rotationPerSec;

    return S_OK;
}

shared_ptr<Transform> Transform::Create(ComPtr<Device> device,
    ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Transform>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Transform");

        return nullptr;
    }

    return instance;
}

shared_ptr<Component> Transform::Clone(any arg)
{
    auto instance = make_shared<Transform>(*this);

    if (FAILED(instance->Initialize(arg))) {
        MSG_BOX("Failed to Clone Transform");

        return nullptr;
    }

    return instance;
}

void Transform::Free()
{
    Component::Free();
}
