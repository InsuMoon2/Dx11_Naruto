#include "pch.h"
#include "MainApp.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "EditorInstance.h"

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
    {
        EDITOR_DESC editorDesc;
        editorDesc.hWnd = g_hWnd;
        editorDesc.winMode = EWinMode::Win;
        editorDesc.viewportWidth = g_winSizeX;
        editorDesc.viewportHeight = g_winSizeY;

        if (FAILED(EDITOR->Initialize_Editor(editorDesc, _device, _context)))
            return E_FAIL;
    }

    if (FAILED(Ready_StartLevel(LevelType::Logo)))
        return E_FAIL;


    return S_OK;
}

void MainApp::Priority_Update(float timeDelta)
{
    if (EDITOR->IsPlaying())
    {
        GAME->Priority_Update_Engine(timeDelta);
    }
}

void MainApp::Update(float timeDelta)
{
    EDITOR->Update_Editor(timeDelta);

    if (EDITOR->IsPlaying())
    {
        GAME->Update_Engine(timeDelta);
    }

}

void MainApp::Late_Update(float timeDelta)
{
    if (EDITOR->IsPlaying())
    {
        GAME->Late_Update_Engine(timeDelta);
    }
}

HRESULT MainApp::Render()
{
    Color clearColor = { 0.3f, 0.3f, 0.3f, 1.f };

    if(FAILED(GAME->Clear_Buffers(clearColor)))
        return E_FAIL;

    //if (FAILED(GAME->Draw()))
    //    return E_FAIL; -> Editor로 이동

    EDITOR->Render_Editor();

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

    EditorInstance::DestroyInstance();
    GameInstance::DestroyInstance();


}
