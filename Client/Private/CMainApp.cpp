#include "pch.h"
#include "CMainApp.h"
#include "CGameInstance.h"
#include "CLevel_Loading.h"

CMainApp::CMainApp()
{
}

CMainApp::~CMainApp()
{

}

HRESULT CMainApp::Initialize()
{
    ENGINE_DESC engineDesc = {};
    engineDesc.hWnd = g_hWnd;           
    engineDesc.winMode = WINMODE::WIN;
    engineDesc.viewportWidth = g_winSizeX;  
    engineDesc.viewportHeight = g_winSizeY;

    if (FAILED(GAME->Initialize_Engine(engineDesc, _device, _context)))
        return E_FAIL;

    GAME->Change_Level(static_cast<uint32>(LEVEL::LOADING), CLevel_Loading::Create(_device, _context));


    return S_OK;
}

void CMainApp::Update(float timeDelta)
{
    GAME->Update_Engine(timeDelta);
}

void CMainApp::LateUpdate(float timeDelta)
{
    GAME->LateUpdate_Engine(timeDelta);
}

HRESULT CMainApp::Render()
{
    color clearColor = { 0.f, 0.f, 1.f, 1.f };

    if(FAILED(GAME->Clear_Buffers(clearColor)))
        return E_FAIL;

    if (FAILED(GAME->Draw()))
        return E_FAIL;

    if (FAILED(GAME->Present()))
        return E_FAIL;

    return S_OK;
}

unique_ptr<CMainApp> CMainApp::Create()
{
    auto instance = make_unique<CMainApp>();

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Created : MainApp");

        return nullptr;
    }

    return instance;
}

void CMainApp::Free()
{
    CBase::Free();

}
