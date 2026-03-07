#include "pch.h"
#include "Editor_MainApp.h"
#include "Texture.h"
#include "Background.h"
#include "EditorInstance.h"
#include "GameInstance.h"
#include "Level_Editor.h"
#include "Level_Loading.h"
#include "ResourceLoader.h"
#include "VIBuffer_Rect.h"
#include "Shader.h"
#include "Event_Manager.h"

Editor_MainApp::Editor_MainApp()
{
    
}

Editor_MainApp::~Editor_MainApp()
{
    
}

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
    CHECK_FAILED(Ready_StartLevel(ELevelType::MainTitle), E_FAIL);

    return S_OK;
}

void Editor_MainApp::Priority_Update(float timeDelta)
{
    if (EDITOR->IsPlaying())
    {
        GAME->Priority_Update_Engine(timeDelta);
    }
    else
    {
        GAME->Update_CameraOnly(timeDelta);
    }
}

void Editor_MainApp::Update(float timeDelta)
{
    INPUT->Update(timeDelta);

    if (GAME->IsPlaying() && INPUT->KeyDown(KEY_TYPE::ESCAPE))
    {
        EDITOR->Stop();
    }

    // 에디터는 항상 업데이트
    EDITOR->Update_Editor(timeDelta);

    // 게임은 Play 모드일 때만
    if (EDITOR->IsPlaying())
        GAME->Update_Engine(timeDelta);

}

void Editor_MainApp::Late_Update(float timeDelta)
{
    // Edit 모드에서도 Late_Update를 돌려야 RenderGroup에 오브젝트가 등록됨
    GAME->Late_Update_Engine(timeDelta);
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
    if (FAILED(GAME->Add_Component_Prototype(ETOI(ELevelType::Static),
        Protocol::COMPONENT_TYPE_SHADER_VTXTEX,
        Shader::Create(_device, _context, TEXT("../../Client/Bin/Shaders/Shader_Vtxtex.hlsl"),
            VTXTEX::Elements, VTXTEX::numElements))))
    {
        return E_FAIL;
    }

    shared_ptr<Texture> loadingTex = Texture::Create(_device, _context, TEXT("../../Client/Bin/Resources/Textures/UI/Loading_Screen/Textures/T_UI_LoadingScreen_%03d_BC.png"), 2);
    if (FAILED(GAME->Add_Component_Prototype(ETOI(ELevelType::Static),
        Protocol::COMPONENT_TYPE_TEXTURE_LOADING, loadingTex)))
    {
        return E_FAIL;
    }

    if (FAILED(GAME->Add_Component_Prototype(ETOI(ELevelType::Static),
        Protocol::COMPONENT_TYPE_RECT, VIBuffer_Rect::Create(_device, _context))))
    {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT Editor_MainApp::Ready_StartLevel(ELevelType startLevelID)
{
    if (ELevelType::Loading == startLevelID)
        return E_FAIL;

    if (FAILED(GAME->Change_Level(ETOI(ELevelType::Loading),
        Level_Loading::Create(_device, _context, startLevelID))))
        return E_FAIL;

    return S_OK;
}

unique_ptr<Editor_MainApp> Editor_MainApp::Create()
{
    auto instance = make_unique<Editor_MainApp>();

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Created : EditorMainApp");

        return nullptr;
    }
    return instance;
}

void Editor_MainApp::Free()
{
    Base::Free();

    EditorInstance::DestroyInstance();
    GameInstance::DestroyInstance();

    Input_Manager::DestroyInstance();
    Event_Manager::DestroyInstance();

    Inspector_Factory::DestroyInstance();
    
}
