#include "MainApp.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "NetworkManager.h"
#include "ResourceLoader.h"
#include "VIBuffer_Rect.h"
#include "pch.h"

MainApp::MainApp() {}

MainApp::~MainApp() {}

HRESULT MainApp::Initialize() {
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

        GAME->Set_EditorRuntime(false);
    }

    CHECK_FAILED(Ready_StaticLevel(), E_FAIL);

    if (FAILED(Ready_StartLevel(ELevelType::GamePlay)))
        return E_FAIL;

    NetworkManager::GetInstance()->Initialize();

    return S_OK;
}

void MainApp::Priority_Update(float timeDelta) {
    GAME->Priority_Update_Engine(timeDelta);
}

void MainApp::Update(float timeDelta) {
    GAME->Update_Engine(timeDelta);
    NetworkManager::GetInstance()->Update();
}

void MainApp::Late_Update(float timeDelta) {
    GAME->Late_Update_Engine(timeDelta);
}

HRESULT MainApp::Render() {
    Color clearColor = { 0.1f, 0.3f, 1.0f, 1.f };

    CHECK_FAILED(GAME->Clear_Buffers(clearColor), E_FAIL);
    CHECK_FAILED(GAME->Draw(), E_FAIL);
    CHECK_FAILED(GAME->Present(), E_FAIL);

    return S_OK;
}

HRESULT MainApp::Ready_StaticLevel() {
    auto resourceLoader = ResourceLoader::Create(_device, _context);
    CHECK_NULL(resourceLoader, E_FAIL);

    CHECK_FAILED(resourceLoader->Load_Table(
        TEXT("../Bin/Resources/Data/json/StaticLevelComTable.json"),
        ETOI(ELevelType::Static)),
        E_FAIL);

    return S_OK;
}

HRESULT MainApp::Ready_StartLevel(ELevelType startLevelID) {
    if (ELevelType::Loading == startLevelID)
        return E_FAIL;

    const bool loadSharedResources = (startLevelID == ELevelType::GamePlay);

    // 서버가 실제로 연결되어 있을 때만 Server 모드로 진입.
    // 서버 없이 단독 테스트 시에는 LocalOnly로 플레이어를 바로 스폰한다.
    const bool isServerConnected = NetworkManager::GetInstance()->IsConnected();
    const bool useServerMode = (startLevelID == ELevelType::GamePlay) && isServerConnected;
    const EGameplaySpawnMode spawnMode =
        useServerMode ? EGameplaySpawnMode::Server
                      : EGameplaySpawnMode::LocalOnly;

    if (FAILED(GAME->Change_Level(
        ETOI(ELevelType::Loading),
        Level_Loading::Create(_device, _context, startLevelID, loadSharedResources, spawnMode))))
        return E_FAIL;

    return S_OK;
}

unique_ptr<MainApp> MainApp::Create() {
    auto instance = make_unique<MainApp>();

    if (FAILED(instance->Initialize())) {
        MSG_BOX("Failed to Created : MainApp");

        return nullptr;
    }

    return instance;
}

void MainApp::Free() {
    Base::Free();

    // Network
    {
        NetworkManager::GetInstance()->Free();
        NetworkManager::DestroyInstance();
    }

    GameInstance::DestroyInstance();
}
