#include "pch.h"
#include "Light.h"

Light::Light()
{

}

HRESULT Light::Initialize(const FLightDesc& desc)
{
    _lightDesc = desc;

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
