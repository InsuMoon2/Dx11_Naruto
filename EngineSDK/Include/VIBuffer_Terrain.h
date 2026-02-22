#pragma once

#include "VIBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL VIBuffer_Terrain : public VIBuffer
{
    GENERATED_COMPONENT(VIBuffer_Terrain, Protocol::COMPONENT_TYPE_RECT)

public:
    explicit VIBuffer_Terrain(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit VIBuffer_Terrain(const VIBuffer_Terrain& rhs);
    virtual ~VIBuffer_Terrain();

public:
    virtual HRESULT Initialize_Prototype(const wstring& heightMapPath);
    virtual HRESULT Initialize(void* arg) override;
    virtual void    BeginPlay() override;

private:
    uint32 _numVerticesX = {};
    uint32 _numVerticesZ = {};

public:
    static Shared<VIBuffer_Terrain> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const wstring& heightMapPath);
    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;
 
};

NS_END
