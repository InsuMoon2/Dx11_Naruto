#pragma once

#include "VIBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL VIBuffer_Rect : public VIBuffer
{
    GENERATED_COMPONENT(VIBuffer_Rect, Protocol::COMPONENT_TYPE_RECT)

public:
    explicit VIBuffer_Rect(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit VIBuffer_Rect(const VIBuffer_Rect& rhs);
    virtual ~VIBuffer_Rect();

public:
    virtual HRESULT Initialize_Prototype();
    virtual HRESULT Initialize(void* pArg);
    void    BeginPlay() override;

public:
    static Shared<VIBuffer_Rect> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<Component> Clone(void* pArg) override;
    virtual void Free() override;

};

NS_END
