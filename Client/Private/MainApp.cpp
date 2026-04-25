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

HRESULT MainApp::Ready_StartLevel(ELevelType startLevelID)
{
    if (ELevelType::Loading == startLevelID)
        return E_FAIL;

    // Gameplay와 Konoha는 동일하게 shared resource + network spawn rule을 사용한다.
    const bool isRuntimeBattleLevel =
        (startLevelID == ELevelType::GamePlay) ||
        (startLevelID == ELevelType::Konoha);

    const bool loadSharedResources = isRuntimeBattleLevel;

    // 에디터 멀티플레이 테스트 실행에서는 battle level일 때 서버 스폰 모드를 사용한다.
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

void MainApp::Free() {
    Base::Free();

    // Network
    {
        NetworkManager::GetInstance()->Free();
        NetworkManager::DestroyInstance();
    }

    GameInstance::DestroyInstance();
}
