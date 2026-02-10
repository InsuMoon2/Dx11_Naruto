#include "pch.h"
#include "Editor_MainApp.h"
#include "EditorInstance.h"
#include "GameInstance.h"
#include "Level_Editor.h"
#include "ResourceLoader.h"
#include "VIBuffer_Rect.h"

Editor_MainApp::Editor_MainApp() {}

Editor_MainApp::~Editor_MainApp() {}

HRESULT Editor_MainApp::Initialize()
{
    // Engine Setting
    {
        ENGINE_DESC engineDesc = {};
        engineDesc.hWnd = g_hWnd;
        engineDesc.winMode = EWinMode::Win;
        engineDesc.viewportWidth = g_winSizeX;
        engineDesc.viewportHeight = g_winSizeY;
        engineDesc.numLevels = ETOI(ELevelType::END);

        if (FAILED(GAME->Initialize_Engine(engineDesc, _device, _context)))
            return E_FAIL;
    }

    // Editor Setting (항상 활성화)
    {
        EDITOR_DESC editorDesc;
        editorDesc.hWnd = g_hWnd;
        editorDesc.winMode = EWinMode::Win;
        editorDesc.viewportWidth = g_winSizeX;
        editorDesc.viewportHeight = g_winSizeY;

        if (FAILED(EDITOR->Initialize_Editor(editorDesc, _device, _context)))
            return E_FAIL;
    }

    CHECK_FAILED(Ready_StaticLevel(), E_FAIL);

    auto emptyLevel = Level_Editor::Create(_device, _context);
    CHECK_NULL(emptyLevel, E_FAIL);

    CHECK_FAILED(GAME->Change_Level(ETOI(ELevelType::GamePlay), emptyLevel),
        E_FAIL);

    return S_OK;
}

void Editor_MainApp::Priority_Update(float timeDelta)
{
    if (EDITOR->IsPlaying()) {
        GAME->Priority_Update_Engine(timeDelta);
    }
}

void Editor_MainApp::Update(float timeDelta)
{
    // 에디터는 항상 업데이트
    EDITOR->Update_Editor(timeDelta);

    // 게임은 Play 모드일 때만
    if (EDITOR->IsPlaying()) {
        GAME->Update_Engine(timeDelta);
    }

    // TODO: NetworkManager::GetInstance()->Update();
}

void Editor_MainApp::Late_Update(float timeDelta)
{
    if (EDITOR->IsPlaying()) {
        GAME->Late_Update_Engine(timeDelta);
    }
}

HRESULT Editor_MainApp::Render()
{
    Color clearColor = { 0.3f, 0.3f, 0.3f, 1.f };

    if (FAILED(GAME->Clear_Buffers(clearColor)))
        return E_FAIL;

    EDITOR->Render_Editor();

    if (FAILED(GAME->Present()))
        return E_FAIL;

    return S_OK;
}

HRESULT Editor_MainApp::Ready_StaticLevel()
{
    auto resourceLoader = ResourceLoader::Create(_device, _context);
    CHECK_NULL(resourceLoader, E_FAIL);

    CHECK_FAILED(resourceLoader->Load_Table(
        TEXT("../Bin/Resources/Data/json/StaticLevelComTable.json"),
        ETOI(ELevelType::Static)),
        E_FAIL);

    return S_OK;
}

unique_ptr<Editor_MainApp> Editor_MainApp::Create()
{
    auto instance = make_unique<Editor_MainApp>();

    if (FAILED(instance->Initialize())) {
        MSG_BOX("Failed to Created : EditorMainApp");

        return nullptr;
    }
    return instance;
}

void Editor_MainApp::Free()
{
    Base::Free();

    // Network
    {
        // NetworkManager::GetInstance()->Free();
        // NetworkManager::DestroyInstance();
    }

    EditorInstance::DestroyInstance();
    GameInstance::DestroyInstance();
}
