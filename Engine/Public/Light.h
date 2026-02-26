#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Light final : public Base
{
public:
    explicit Light();
    virtual ~Light() = default;

public:
    HRESULT Initialize(const FLightDesc& desc);

    const FLightDesc* Get_LightDesc() { return &_lightDesc; }

private:
    FLightDesc _lightDesc {};

public:
    static Shared<Light> Create(const FLightDesc& desc);
    void Free() override;

};

NS_END
