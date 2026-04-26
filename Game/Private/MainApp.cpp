#include "pch.h"
#include "MainApp.h"

#include "Customizer_Manager.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "Loader.h"
#include "NetworkManager.h"
#include "VIBuffer_Rect.h"
#include "Event_Manager.h"
#include "ResourceLoader.h"

#include <ctime>
#include <cstdlib>

#include "ComboProfile_Manager.h"

MainApp::MainApp(bool startInServerGameplayMode)
    : _startInServerGameplayMode(startInServerGameplayMode)
{
}

MainApp::~MainApp()
{

}

HRESULT MainApp::Initialize()
{
    srand(static_cast<unsigned int>(time(nullptr)));

    // Engine Setting
    {
        RECT clientRect;
        GetClientRect(g_hWnd, &clientRect);
        unsigned int realWidth = clientRect.right - clientRect.left;
        unsigned int realHeight = clientRect.bottom - clientRect.top;

        ENGINE_DESC engineDesc = {};
        engineDesc.hWnd = g_hWnd;
        engineDesc.winMode = EWinMode::Win;

        engineDesc.viewportWidth = realWidth;
        engineDesc.viewportHeight = realHeight;

        engineDesc.numLevels = ETOI(ELevelType::END);

        engineDesc.uiReferenceWidth = 1920;
        engineDesc.uiReferenceHeight = 1080;

        GAME->Set_UIViewportSize((float)realWidth, (float)realHeight);

        if (FAILED(GAME->Initialize_Engine(engineDesc, _device, _context)))
            return E_FAIL;

        GAME->Set_UIPrototypeLevel(ETOI(ELevelType::Static));
        GAME->Set_EditorRuntime(false);
    }

    CHECK_FAILED(Ready_StaticLevel(), E_FAIL);

    if (_startInServerGameplayMode)
    {
        NetworkManager::GetInstance()->Initialize();
        CHECK_FAILED(Ready_StartLevel(ELevelType::MainTitle), E_FAIL);
    }
    else
    {
        CHECK_FAILED(Ready_StartLevel(ELevelType::MainTitle), E_FAIL);
    }

    GAME->Set_GameState(EGameState::Play);
    GAME->Set_GameInputEnabled(true);

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
    CHECK_FAILED(GAME->Draw(false, false, false), E_FAIL);
    CHECK_FAILED(GAME->Present(), E_FAIL);

    return S_OK;
}

HRESULT MainApp::Ready_StaticLevel()
{
    auto resourceLoader = ResourceLoader::Create(_device, _context);
    CHECK_NULL(resourceLoader, E_FAIL);

    CHECK_FAILED(resourceLoader->Load_ShaderTable(
        TEXT("../../Client/Bin/Resources/Data/json/DT_Shader.json")), E_FAIL);

    CHECK_FAILED(resourceLoader->Load_TextureTable(
        TEXT("../../Client/Bin/Resources/Data/json/DT_Texture.json")), E_FAIL);

    Loader componentRegistrar(_device, _context);
    componentRegistrar.Register_Components();
    componentRegistrar.Initialize_BT_Nodes();

    CHECK_FAILED(GAME->Add_Component_Prototype(
        ETOI(ELevelType::Static),
        Protocol::COMPONENT_TYPE_RECT,
        VIBuffer_Rect::Create(_device, _context)), E_FAIL);

    vector<FLoadJob> jobs;

    CHECK_FAILED(resourceLoader->Build_AllResourceJobs(
        TEXT("../../Client/Bin/Resources/Data/json/DT_GameObject.json"), jobs), E_FAIL);

    for (auto& job : jobs)
    {
        if (job.type != ELoadJobType::GameObjectPrototype)
            continue;

        auto instance = GAME->Create_GameObjectFromFactory(
            static_cast<Protocol::OBJECT_TYPE>(job.objectType));

        if (!instance)
            continue;

        CHECK_FAILED(
            GAME->Add_GameObject_Prototype(job.levelIndex, job.objectType, instance),
            E_FAIL);
    }
   
    return S_OK;
}

HRESULT MainApp::Ready_StartLevel(ELevelType startLevelID)
{
    if (ELevelType::Loading == startLevelID)
        return E_FAIL;

    const bool isRuntimeBattleLevel =
        (startLevelID == ELevelType::GamePlay) ||
        (startLevelID == ELevelType::Konoha);

    const bool loadSharedResources =
        startLevelID == ELevelType::MainTitle ||
        startLevelID == ELevelType::CharacterSetup ||
        isRuntimeBattleLevel;

    const bool useServerMode = isRuntimeBattleLevel && _startInServerGameplayMode;
    const EGameplaySpawnMode spawnMode =
        useServerMode ? EGameplaySpawnMode::Server
                      : EGameplaySpawnMode::LocalOnly;

    if (FAILED(GAME->Change_Level(
        ETOI(ELevelType::Loading),
        Level_Loading::Create(_device, _context, startLevelID, loadSharedResources, spawnMode))))
    {
        return E_FAIL;
    }

    return S_OK;
}


unique_ptr<MainApp> MainApp::Create(bool startInServerGameplayMode)
{
    auto instance = make_unique<MainApp>(startInServerGameplayMode);

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
    Customizer_Manager::DestroyInstance();

    ComboProfile_Manager::DestroyInstance();
}
