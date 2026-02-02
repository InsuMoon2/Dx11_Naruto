#include "pch.h"
#include "MainApp.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "EditorInstance.h"
#include "NetworkManager.h"

MainApp::MainApp()
{
}

MainApp::~MainApp()
{

}

HRESULT MainApp::Initialize()
{
    // Engine Setting
    {
        ENGINE_DESC engineDesc = {};
        engineDesc.hWnd = g_hWnd;
        engineDesc.winMode = EWinMode::Win;
        engineDesc.viewportWidth = g_winSizeX;
        engineDesc.viewportHeight = g_winSizeY;
        engineDesc.numLevels = ETOI(LevelType::END);

        if (FAILED(GAME->Initialize_Engine(engineDesc, _device, _context)))
            return E_FAIL;
    }

    // Editor Setting
    if (g_enableEditor)
    {
        EDITOR_DESC editorDesc;
        editorDesc.hWnd = g_hWnd;
        editorDesc.winMode = EWinMode::Win;
        editorDesc.viewportWidth = g_winSizeX;
        editorDesc.viewportHeight = g_winSizeY;

        if (FAILED(EDITOR->Initialize_Editor(editorDesc, _device, _context)))
            return E_FAIL;
    }

    if (FAILED(Ready_StartLevel(LevelType::GamePlay)))
        return E_FAIL;

	NetworkManager::GetInstance()->Initialize();

    return S_OK;
}

void MainApp::Priority_Update(float timeDelta)
{
    if (!g_enableEditor)
    {
        GAME->Priority_Update_Engine(timeDelta);
    }
    else if (EDITOR->IsPlaying()) 
    {
        GAME->Priority_Update_Engine(timeDelta);
    }
}

void MainApp::Update(float timeDelta)
{
    if (g_enableEditor)
    {
        EDITOR->Update_Editor(timeDelta);
    }

    if (!g_enableEditor)
    {
        GAME->Update_Engine(timeDelta);
    }
    else if (EDITOR->IsPlaying())  
    {
        GAME->Update_Engine(timeDelta);
    }

    NetworkManager::GetInstance()->Update();
}

void MainApp::Late_Update(float timeDelta)
{
    if (!g_enableEditor)
    {
        GAME->Late_Update_Engine(timeDelta);
    }
    else if (EDITOR->IsPlaying())
    {
        GAME->Late_Update_Engine(timeDelta);
    }
}

HRESULT MainApp::Render()
{
    Color clearColor = g_enableEditor ?
        Color{ 0.3f, 0.3f, 0.3f, 1.f } :  // 에디터: 회색
        Color{ 0.1f, 0.3f, 1.0f, 1.f };   // 게임: 파란색

    if(FAILED(GAME->Clear_Buffers(clearColor)))
        return E_FAIL;

    if (g_enableEditor) // 에디터 모드
    {
        EDITOR->Render_Editor();
    }
    else                // 게임 모드
    {
        if (FAILED(GAME->Draw()))
            return E_FAIL;
    }

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

	// Network
	{
		NetworkManager::GetInstance()->Free();
		NetworkManager::DestroyInstance();
	}

    EditorInstance::DestroyInstance();
    GameInstance::DestroyInstance();

}
