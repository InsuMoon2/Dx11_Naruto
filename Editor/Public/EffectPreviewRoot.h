#pragma once

#include "GameObject.h"

NS_BEGIN(Editor)

class EffectPreviewRoot final : public GameObject
{
    GENERATED_BODY(EffectPreviewRoot)

public:
    explicit EffectPreviewRoot(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit EffectPreviewRoot(const EffectPreviewRoot& rhs);
    virtual ~EffectPreviewRoot() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual void    Priority_Update(float timeDelta) override;
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;
};

NS_END
