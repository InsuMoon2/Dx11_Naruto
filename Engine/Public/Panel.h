#pragma once
#include "HUD.h"

NS_BEGIN(Engine)

// HUD와 사실상 역할은 동일하지만 포커스 가능 여부 정도 차이
class ENGINE_DLL Panel : public HUD
{
    GENERATED_BODY(Panel)

public:
    explicit Panel(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Panel(const Panel& rhs);
    virtual ~Panel() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;

    virtual bool Is_Focusable() const override { return true; }

public:
    virtual void Free() override;
};

NS_END
