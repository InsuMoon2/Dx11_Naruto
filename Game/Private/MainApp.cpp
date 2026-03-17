#include "pch.h"
#include "MainApp.h"

#include "Background.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "NetworkManager.h"
#include "VIBuffer_Rect.h"
#include "Shader.h"
#include "Texture.h"
#include "Event_Manager.h"
#include "ResourceLoader.h"
#include "StaticMeshActor.h"
#include "UI_LoadingProgressBar.h"
#include "UI_LoadingSpinner.h"
#include "UI_PlayerHP.h"
#include "UI_PlayerHUD.h"
#include "UI_PlayerSkill.h"
#include "UI_PlayerStatus.h"
#include "UI_SkillSlot.h"
#include "UI_MainTitleMenuButton.h"
#include "UI_Text.h"

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
        engineDesc.numLevels = ETOI(ELevelType::END);

        if (FAILED(GAME->Initialize_Engine(engineDesc, _device, _context)))
            return E_FAIL;

        GAME->Set_UIPrototypeLevel(ETOI(ELevelType::Static));
    }

    CHECK_FAILED(Ready_StaticLevel(), E_FAIL);
    CHECK_FAILED(Ready_StartLevel(ELevelType::MainTitle), E_FAIL);

    GAME->Set_GameState(EGameState::Play);

    return S_OK;
}

void MainApp::Priority_Update(float timeDelta)
{
    GAME->Priority_Update_Engine(timeDelta);
}

void MainApp::Update(float timeDelta)
{
    INPUT->Update(timeDelta);

    GAME->Update_Engine(timeDelta);

    if (!_networkConnected && GAME->Current_Level() == ETOI(ELevelType::GamePlay))
    {
        NetworkManager::GetInstance()->Initialize();
        _networkConnected = true;
    }

    NetworkManager::GetInstance()->Update();
}

void MainApp::Late_Update(float timeDelta)
{
    GAME->Late_Update_Engine(timeDelta);
}

HRESULT MainApp::Render()
{
    Color clearColor = { 0.1f, 0.3f, 1.0f, 1.f };

    CHECK_FAILED(GAME->Clear_Buffers(clearColor), E_FAIL);
    CHECK_FAILED(GAME->Draw(), E_FAIL);
    CHECK_FAILED(GAME->Present(), E_FAIL);

    return S_OK;
}

HRESULT MainApp::Ready_StaticLevel()
{
    auto resourceLoader = ResourceLoader::Create(_device, _context);
    CHECK_NULL(resourceLoader, E_FAIL);

    CHECK_FAILED(resourceLoader->Load_ShaderTable(
        TEXT("../../Client/Bin/Resources/Data/json/ShaderTable.json")), E_FAIL);

    CHECK_FAILED(resourceLoader->Load_TerrainTable(
        TEXT("../../Client/Bin/Resources/Data/json/TerrainTable.json")), E_FAIL);

    CHECK_FAILED(resourceLoader->Load_TextureTable(
        TEXT("../../Client/Bin/Resources/Data/json/TextureTable.json")), E_FAIL);

    CHECK_FAILED(GAME->Add_Component_Prototype(
        ETOI(ELevelType::Static),
        Protocol::COMPONENT_TYPE_RECT,
        VIBuffer_Rect::Create(_device, _context)), E_FAIL);

    if (FAILED(GAME->Add_GameObject_Prototype(ETOI(ELevelType::Static),
        Protocol::OBJECT_TYPE_STATIC_MESH,
        StaticMeshActor::Create(_device, _context))))
    {
        return E_FAIL;
    }

    // UI
    const uint32 staticLevel = ETOI(ELevelType::Static);

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_BACKGROUND,
        Background::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_UI_TEXT,
        UI_Text::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_UI_LOADING_SPINNER,
        UI_LoadingSpinner::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_UI_LOADING_PROGRESS_BAR,
        UI_LoadingProgressBar::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_UI_PLAYER_HP,
        UI_PlayerHP::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_UI_SKILL_SLOT,
        UI_SkillSlot::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_UI_PLAYER_STATUS,
        UI_PlayerStatus::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_UI_PLAYER_SKILL,
        UI_PlayerSkill::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_UI_PLAYER_HUD,
        UI_PlayerHUD::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_UI_MAIN_TITLE_TEXT,
        UI_MainTitleMenuButton::Create(_device, _context));

    return S_OK;
}

HRESULT MainApp::Ready_StartLevel(ELevelType startLevelID)
{
    if (ELevelType::Loading == startLevelID)
        return E_FAIL;

    const bool loadSharedResources = (startLevelID == ELevelType::GamePlay);

    if (FAILED(GAME->Change_Level(ETOI(ELevelType::Loading),
        Level_Loading::Create(_device, _context, startLevelID, loadSharedResources))))
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

    GameInstance::DestroyInstance();

    Input_Manager::DestroyInstance();
    Event_Manager::DestroyInstance();

}
