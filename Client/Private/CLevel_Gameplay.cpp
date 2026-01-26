#include "pch.h"
#include "CLevel_Gameplay.h"
#include "CLoader.h"
#include "CGameInstance.h"
#include "CLevel_Loading.h"

CLevel_Gameplay::CLevel_Gameplay(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : CLevel { device, context }
{
}

CLevel_Gameplay::~CLevel_Gameplay()
{
}

HRESULT CLevel_Gameplay::Initialize()
{

    return S_OK;
}

void CLevel_Gameplay::Update(float timeDelta)
{
    CLevel::Update(timeDelta);

}

void CLevel_Gameplay::LateUpdate(float timeDelta)
{
    CLevel::LateUpdate(timeDelta);

}

HRESULT CLevel_Gameplay::Render()
{
    #ifdef _DEBUG
    SetWindowText(g_hWnd, TEXT("현재 레벨 : GamePlay"));
    #endif
    

    return S_OK;
}

shared_ptr<CLevel_Gameplay> CLevel_Gameplay::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<CLevel_Gameplay>(device, context);

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Create : Level_GamePlay");

        return nullptr;
    }

    return instance;
}

void CLevel_Gameplay::Free()
{
    CLevel::Free();

}

