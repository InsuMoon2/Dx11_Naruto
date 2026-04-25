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

HRESULT Light::Update_Desc(const FLightDesc& desc)
{
    _lightDesc = desc;
    return S_OK;
}

HRESULT Light::Render(Shared<Shader> shader, Shared<VIBuffer_Rect> viBuffer)
{
    uint32 type = static_cast<uint32>(_lightDesc.type);

    uint32 shaderPass = {};

    const int lightCastsShadow =
        (_lightDesc.type == ELightType::Directional && _lightDesc.castShadow) ? 1 : 0;

    if (FAILED(shader->Bind_RawValue("g_LightCastsShadow", &lightCastsShadow, sizeof(int))))
        return E_FAIL;

    if (_lightDesc.type == ELightType::Directional)
    {
        if (FAILED(shader->Bind_RawValue("g_ShadowBias", &_lightDesc.shadowBias, sizeof(float))))
            return E_FAIL;

        if (FAILED(shader->Bind_RawValue("g_ShadowStrength", &_lightDesc.shadowStrength, sizeof(float))))
            return E_FAIL;

        if (FAILED(shader->Bind_RawValue("g_ShadowSoftness", &_lightDesc.shadowSoftness, sizeof(float))))
            return E_FAIL;

        if (FAILED(shader->Bind_RawValue("g_LightDir", &_lightDesc.direction, sizeof _lightDesc.direction)))
            return E_FAIL;

        shaderPass = ETOI(EDeferred::Directinal);
    }
    else if (_lightDesc.type == ELightType::Point)
	{
        // 정광원은 위치, 범위가 있어야한다.
		if (FAILED(shader->Bind_RawValue("g_LightPos", &_lightDesc.position, sizeof _lightDesc.position)))
            return E_FAIL;

        if (FAILED(shader->Bind_RawValue("g_LightRange", &_lightDesc.range, sizeof _lightDesc.range)))
            return E_FAIL;

		shaderPass = ETOI(EDeferred::Point);
	}

    if (FAILED(shader->Bind_RawValue("g_LightDiffuse", &_lightDesc.diffuse, sizeof _lightDesc.diffuse)))
        return E_FAIL;

    if (FAILED(shader->Bind_RawValue("g_LightAmbient", &_lightDesc.ambient, sizeof _lightDesc.ambient)))
        return E_FAIL;

    if (FAILED(shader->Bind_RawValue("g_LightSpecular", &_lightDesc.specular, sizeof _lightDesc.specular)))
        return E_FAIL;

    CHECK_FAILED(shader->Begin_Pass(shaderPass), E_FAIL);

    return viBuffer->Render();
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
