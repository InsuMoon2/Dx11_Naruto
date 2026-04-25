#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Light;
class Shader;
class VIBuffer_Rect;

class Light_Manager : public Base
{
public:
    explicit Light_Manager();
    virtual ~Light_Manager() = default;

public:
    const FLightDesc* Get_LightDesc(uint32 index);
    HRESULT           Add_Light(const FLightDesc& desc);
    // 현재 primary shadow directional light의 설정을 갱신할 때 호출한다.
    HRESULT           Update_PrimaryShadowLightDesc(const FLightDesc& desc);
    void              Clear_Lights();

    HRESULT           Render_Lights(Shared<Shader> shader, Shared<VIBuffer_Rect> viBuffer);

    const FLightDesc* Get_PrimaryShadowLightDesc() const;

private:
    vector<Shared<Light>> _lights;

public:
    static Unique<Light_Manager> Create();
    void Free() override;

};

NS_END
