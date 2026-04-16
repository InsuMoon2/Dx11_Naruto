#include "pch.h"
#include "Light.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"

Light::Light()
{

}

HRESULT Light::Initialize(const FLightDesc& desc)
{
    _lightDesc = desc;

    return S_OK;
}

HRESULT Light::Render(Shared<Shader> shader, Shared<VIBuffer_Rect> viBuffer)
{
    uint32 type = static_cast<uint32>(_lightDesc.type);

    if (FAILED(shader->Bind_RawValue("g_LightType", &type, sizeof(uint32))))
        return E_FAIL;
    
    if (FAILED(shader->Bind_RawValue("g_LightDir", &_lightDesc.direction, sizeof(Vec4))))
        return E_FAIL;

    if (FAILED(shader->Bind_RawValue("g_LightPos", &_lightDesc.position, sizeof(Vec4))))
        return E_FAIL;

    if (FAILED(shader->Bind_RawValue("g_LightRange", &_lightDesc.range, sizeof(float))))
        return E_FAIL;

    if (FAILED(shader->Bind_RawValue("g_LightDiffuse", &_lightDesc.diffuse, sizeof(Vec4))))
        return E_FAIL;

    if (FAILED(shader->Bind_RawValue("g_LightAmbient", &_lightDesc.ambient, sizeof(Vec4))))
        return E_FAIL;

    if (FAILED(shader->Bind_RawValue("g_LightSpecular", &_lightDesc.specular, sizeof(Vec4))))
        return E_FAIL;

    shader->Begin_Pass(1);

    viBuffer->Render();

    return S_OK;
}

Shared<Light> Light::Create(const FLightDesc& desc)
{
    auto instance = make_shared<Light>();

    if (FAILED(instance->Initialize(desc)))
    {
        MSG_BOX("Failed to Create : Light");
        instance->Free();

        return nullptr;
    }

    return instance;
}

void Light::Free()
{
    Base::Free();
}
