#pragma once

#include "VIBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL VIBuffer_Instance : public VIBuffer
{
public:
    struct FInstanceDesc
    {
        uint32  numInstances = 0; 
        Vec3    center = Vec3::Zero;
        Vec3    range = Vec3::Zero; 
        Vec2    scale = Vec2(1.f, 1.f); // 랜덤 스케일 때문에 Vec2
    };

protected:
    explicit VIBuffer_Instance(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit VIBuffer_Instance(const VIBuffer_Instance& rhs);
    virtual ~VIBuffer_Instance() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual HRESULT Bind_Resources();
    virtual HRESULT Render();

protected:
    ComPtr<Buffer>      _instanceBuffer;
    D3D11_BUFFER_DESC   _instanceBufferDesc{};

    uint32              _instanceStride;
    uint32              _numInstances = 0;
    uint32              _indexCountPerInstance = 0;

public:
    virtual Shared<Component> Clone(void* arg) override = 0;
    virtual void Free() override;

};

NS_END
