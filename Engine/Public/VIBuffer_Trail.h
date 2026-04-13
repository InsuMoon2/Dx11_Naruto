#pragma once

#include "VIBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL VIBuffer_Trail : public VIBuffer
{
    GENERATED_COMPONENT(VIBuffer_Trail, Protocol::COMPONENT_TYPE_VIBUFFER_TRAIL)

public:
    explicit VIBuffer_Trail(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit VIBuffer_Trail(const VIBuffer_Trail& rhs);
    virtual ~VIBuffer_Trail() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    HRESULT Update_Trail(const deque<FTrailPoint>& points);


private:
    uint32 _maxPoints = 200; // 버퍼 최대 길이 방어용

public:
    static Shared<VIBuffer_Trail> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;
};

NS_END
