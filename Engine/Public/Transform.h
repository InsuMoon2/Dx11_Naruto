#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL Transform : public Component
{
public:
    struct FTransformDesc
    {
        float speedPerSec = {};
        float rotationPerSec = {};
    };

public:
    Transform(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Transform(const Transform& protoType);
    virtual ~Transform();

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(any arg) override;

private:
    Matrix  _worldMatrix = {};
    float   _speedPerSec = {};
    float   _rotationPerSec = {};

public:
    static shared_ptr<Transform> Create(ComPtr<Device> device,ComPtr<DeviceContext> context);
    virtual shared_ptr<Component> Clone(any arg) override;
    virtual void Free() override;

};

NS_END
