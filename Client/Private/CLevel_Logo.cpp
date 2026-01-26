#include "pch.h"
#include "CLevel_Logo.h"
#include "CLoader.h"
#include "CGameInstance.h"
#include "CLevel_Loading.h"

CLevel_Logo::CLevel_Logo(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : CLevel { device, context }
{
}

CLevel_Logo::~CLevel_Logo()
{
}

HRESULT CLevel_Logo::Initialize()
{


    return S_OK;
}

void CLevel_Logo::Update(float timeDelta)
{
    CLevel::Update(timeDelta);

    if (GetKeyState(VK_SPACE) & 0x8000)
    {
        GAME->Change_Level(ETOI(LEVEL::LOADING), CLevel_Loading::Create(_device, _context, LEVEL::GAMEPLAY));
    }
}

void CLevel_Logo::LateUpdate(float timeDelta)
{
    CLevel::LateUpdate(timeDelta);

}

HRESULT CLevel_Logo::Render()
{
    #ifdef _DEBUG
    SetWindowText(g_hWnd, TEXT("현재 레벨 : Logo"));
    #endif
    

    return S_OK;
}

shared_ptr<CLevel_Logo> CLevel_Logo::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<CLevel_Logo>(device, context);

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Create : Level_Logo");

        return nullptr;
    }

    return instance;
}

void CLevel_Logo::Free()
{
    CLevel::Free();

}

