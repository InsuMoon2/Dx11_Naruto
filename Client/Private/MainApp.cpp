#include "pch.h"
#include "MainApp.h"
#include "GameInstance.h"
#include "Level_Loading.h"

MainApp::MainApp()
{
}

MainApp::~MainApp()
{

}

HRESULT MainApp::Initialize()
{
    ENGINE_DESC engineDesc = {};
    engineDesc.hWnd = g_hWnd;           
    engineDesc.winMode = WinMode::Win;
    engineDesc.viewportWidth = g_winSizeX;  
    engineDesc.viewportHeight = g_winSizeY;
    engineDesc.numLevels = ETOI(LevelType::END);

    if (FAILED(GAME->Initialize_Engine(engineDesc, _device, _context)))
        return E_FAIL;

    if (FAILED(Ready_StartLevel(LevelType::Logo)))
        return E_FAIL;


    return S_OK;
}

void MainApp::Priority_Update(float timeDelta)
{
    GAME->Priority_Update(timeDelta);
}

void MainApp::Update(float timeDelta)
{
    GAME->Update_Engine(timeDelta);
}

void MainApp::Late_Update(float timeDelta)
{
    GAME->LateUpdate_Engine(timeDelta);
}

HRESULT MainApp::Render()
{
    Color clearColor = { 0.f, 0.f, 1.f, 1.f };

    if(FAILED(GAME->Clear_Buffers(clearColor)))
        return E_FAIL;

    if (FAILED(GAME->Draw()))
        return E_FAIL;

    if (FAILED(GAME->Present()))
        return E_FAIL;

    return S_OK;
}

HRESULT MainApp::Ready_StartLevel(LevelType startLevelID)
{
    if (LevelType::Loading == startLevelID)
        return E_FAIL;

    if (FAILED(GAME->Change_Level(ETOI(LevelType::Loading), Level_Loading::Create(_device, _context, startLevelID))))
        return E_FAIL;

    return S_OK;
}

unique_ptr<MainApp> MainApp::Create()
{
    auto instance = make_unique<MainApp>();

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Created : MainApp");

        return nullptr;
    }

    return instance;
}

void MainApp::Free()
{
    Base::Free();

}
