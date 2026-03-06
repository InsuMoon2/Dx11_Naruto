#pragma once
#include "UIObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL HUD : public UIObject
{
    GENERATED_BODY(HUD)

public:
    explicit HUD(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit HUD(const HUD& rhs);
    virtual ~HUD() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

public:
    virtual void Free() override;

};

NS_END
