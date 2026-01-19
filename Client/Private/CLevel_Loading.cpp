#include "pch.h"
#include "CLevel_Loading.h"

CLevel_Loading::CLevel_Loading(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : CLevel { device, context }
{
}

CLevel_Loading::~CLevel_Loading()
{
}

HRESULT CLevel_Loading::Initialize()
{

    return S_OK;
}

void CLevel_Loading::Update(float timeDelta)
{
    int a = 10; // Test
}

void CLevel_Loading::LateUpdate(float timeDelta)
{
    int a = 20;
}

HRESULT CLevel_Loading::Render()
{

    return S_OK;
}

shared_ptr<CLevel_Loading> CLevel_Loading::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<CLevel_Loading>(device, context);

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Faield to Created : Level Loading");

        return nullptr;
    }

    return instance;
}

void CLevel_Loading::Free()
{
    CLevel::Free();
}
