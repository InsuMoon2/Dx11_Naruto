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

Unique<Light_Manager> Light_Manager::Create()
{
    return make_unique<Light_Manager>();
}

void Light_Manager::Free()
{
    Base::Free();

    _lights.clear();
}
