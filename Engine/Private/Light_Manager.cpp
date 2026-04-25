#include "pch.h"
#include "Light_Manager.h"
#include "Light.h"

Light_Manager::Light_Manager()
{
}

const FLightDesc* Light_Manager::Get_LightDesc(uint32 index)
{
    if (index >= _lights.size())
        return nullptr;

    return _lights[index]->Get_LightDesc();
}

HRESULT Light_Manager::Add_Light(const FLightDesc& desc)
{
    auto light = Light::Create(desc);
    CHECK_NULL(light, E_FAIL);

    _lights.push_back(light);

    return S_OK;
}

HRESULT Light_Manager::Update_PrimaryShadowLightDesc(const FLightDesc& desc)
{
    Shared<Light> firstDirectionalLight = nullptr; // castShadow가 꺼진 상태에서도 기존 directional light를 다시 찾기 위한 fallback이다.

    for (auto& light : _lights)
    {
        if (!light)
            continue;

        const FLightDesc* currentDesc = light->Get_LightDesc();
        if (!currentDesc)
            continue;

        if (!firstDirectionalLight && currentDesc->type == ELightType::Directional)
            firstDirectionalLight = light;

        if (currentDesc->type == ELightType::Directional && currentDesc->castShadow)
            return light->Update_Desc(desc);
    }

    // castShadow를 false로 바꾼 뒤 다시 true로 켜는 경우에도 기존 directional light를 재사용해야
    // additive lighting pass에 같은 light가 중복으로 쌓이지 않는다.
    if (firstDirectionalLight)
        return firstDirectionalLight->Update_Desc(desc);

    return Add_Light(desc);
}

void Light_Manager::Clear_Lights()
{
    _lights.clear();
}

HRESULT Light_Manager::Render_Lights(Shared<Shader> shader, Shared<VIBuffer_Rect> viBuffer)
{
    for (auto& light : _lights)
    {
        if (light)
            light->Render(shader, viBuffer);
    }

    return S_OK;
}

const FLightDesc* Light_Manager::Get_PrimaryShadowLightDesc() const
{
    for (const auto& light : _lights)
    {
        if (!light)
            continue;

        const FLightDesc* desc = light->Get_LightDesc();
        if (!desc)
            continue;

        if (desc->type == ELightType::Directional && desc->castShadow)
            return desc;
    }

    return nullptr;
}

Unique<Light_Manager> Light_Manager::Create()
{
    return make_unique<Light_Manager>();
}

void Light_Manager::Free()
{
    Base::Free();

    _lights.clear();
}
