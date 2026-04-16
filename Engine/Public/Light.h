#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Shader;
class VIBuffer_Rect;

class Light final : public Base
{
public:
    explicit Light();
    virtual ~Light() = default;

public:
    HRESULT Initialize(const FLightDesc& desc);
    HRESULT Render(Shared<Shader> shader, Shared<VIBuffer_Rect> viBuffer);

    const FLightDesc* Get_LightDesc() { return &_lightDesc; }

private:
    FLightDesc _lightDesc {};

public:
    static Shared<Light> Create(const FLightDesc& desc);
    void Free() override;

};

NS_END
