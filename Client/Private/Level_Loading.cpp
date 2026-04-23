#include "pch.h"
#include "Level_Loading.h"

#include "Background.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_CharacterSetup.h"
#include "Level_Gameplay.h"
#include "Level_Konoha.h"
#include "Level_MainTitle.h"
#include "Level_Lobby.h"
#include "UI_LoadingSpinner.h"
#include "UI_LoadingProgressBar.h"
#include <chrono>

// 메인 스레드에서 오래 걸릴 수 있는 로딩 잡인지 판별한다.
// 로딩 UI가 끊기지 않도록, 이런 잡은 한 프레임에 많이 처리하지 않는다.
static bool Is_HeavyMainThreadLoadJob(const FLoadJob& job)
{
    switch (job.type)
    {
    case ELoadJobType::TextureCreate:
    case ELoadJobType::StaticModelPrototype:
    case ELoadJobType::LevelChunk:
        return true;

    default:
        return false;
    }
}

Level_Loading::Level_Loading(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

Level_Loading::~Level_Loading()
{
}

HRESULT Level_Loading::Initialize(ELevelType nextLevelID, bool loadSharedResources, EGameplaySpawnMode spawnMode)
{
    _nextLevelID = nextLevelID;
    _loadSharedResources = loadSharedResources;
    _gameplaySpawnMode = spawnMode;

    if (FAILED(Ready_Layer_UI(TEXT("Layer_UI"))))
        return E_FAIL;

    // 로더 생성
    _loader = Loader::Create(_device, _context, nextLevelID, _loadSharedResources);
    CHECK_NULL(_loader, E_FAIL);

    auto progressBar = dynamic_pointer_cast<UI_LoadingProgressBar>(_loadingProgressBar);
    if (progressBar)
    {
        progressBar->Bind_Loader(_loader);
    }

    return S_OK;
}

void Level_Loading::Update(float timeDelta)
{
    // 한 프레임에 너무 많은 잡을 처리하면 로딩 스피너가 끊기므로 전체 처리 개수를 줄인다.
    const int32 maxJobsPerFrame = 4;
    // 무거운 잡은 프레임당 1개만 처리해서 큰 프레임 드랍이 연속으로 발생하지 않게 한다.
    const int32 maxHeavyJobsPerFrame = 1;
    // 스피너와 로딩 UI가 계속 갱신되도록 메인 스레드 로딩 시간 예산을 제한한다.
    const auto maxMainThreadLoadingTime = std::chrono::milliseconds(2);
    // 이번 프레임에서 로딩 잡 처리에 쓴 시간을 재기 위한 시작 시각이다.
    const auto frameLoadStart = std::chrono::steady_clock::now();
    int32 processedHeavyJobs = 0;

    for (int32 i = 0; i < maxJobsPerFrame; ++i)
    {
        const auto elapsedLoadingTime = std::chrono::steady_clock::now() - frameLoadStart;
        if (elapsedLoadingTime >= maxMainThreadLoadingTime)
            break;

        FLoadJob job{};
        if (!_loader->Pop_NextJob(job))
            break;

        if (FAILED(_loader->Execute_Job_OnMainThread(job)))
        {
            MSG_BOX("Failed to execute loading job");
            return;
        }

        if (Is_HeavyMainThreadLoadJob(job))
        {
            ++processedHeavyJobs;

            if (processedHeavyJobs >= maxHeavyJobsPerFrame)
                break;
        }
    }

    if (_loader->IsFinished())
    {
        shared_ptr<Level> nextLevel = { nullptr };

        switch (_nextLevelID)
        {
        case ELevelType::MainTitle:
            nextLevel = Level_MainTitle::Create(_device, _context);
            break;

        case ELevelType::GamePlay:
            nextLevel = Level_Gameplay::Create(_device, _context, _gameplaySpawnMode);
            break;

        case ELevelType::CharacterSetup:
            nextLevel = Level_CharacterSetup::Create(_device, _context);
            break;

        case ELevelType::Konoha:
            nextLevel = Level_Konoha::Create(_device, _context, _gameplaySpawnMode);
            break;

        case ELevelType::Lobby:
            nextLevel = Level_Lobby::Create(_device, _context);
            break;
        }

        if (nextLevel == nullptr)
        {
            MSG_BOX("Failed to Created : NextLevel");
            return;
        }

        if (FAILED(GAME->Change_Level(ETOI(_nextLevelID), nextLevel)))
        {
            MSG_BOX("Failed to Change : NextLevel");
            return;
        }

        // 로딩 잡으로 미리 쌓아둔 level / proxy 오브젝트를 현재 레벨 기준 캐시에 다시 반영한다.
        // Konoha / Gameplay는 여기서 collision proxy cache를 한 번 더 재빌드해서
        // 플레이어 MovementComponent가 최신 wall / ground 목록을 받도록 맞춘다.
        auto currentLevel = GAME->Get_Current_Level();
        if (currentLevel)
        {
            currentLevel->On_LevelChunkLoaded(L"PostLoadingFinalize");
        }

        return;
    }
}

void Level_Loading::Late_Update(float timeDelta)
{
    
}

HRESULT Level_Loading::Render()
{
    #ifdef _DEBUG
    _loader->Print_LoadingText();
    #endif

    return S_OK;
}

HRESULT Level_Loading::Ready_Layer_UI(const wstring& uiTag)
{
    Vec2 viewport = Vec2(GAME->Get_UIReferenceWidth(), GAME->Get_UIReferenceHeight());

    const uint32 randomOffset = rand() % BACK_GROUND_COUNT;
    const uint32 selectedBackgroundIndex = ETOI(ELoadingTexture::MainLoading) + randomOffset;

    // 로딩 백그라운드
    {
        Background::FBackgroundDesc desc{};
        desc.name = TEXT("Loading Screen");
        desc.posX = viewport.x * 0.5f;
        desc.posY = viewport.y * 0.5f;
        desc.sizeX = viewport.x;
        desc.sizeY = viewport.y;

        desc.levelIndex = ETOI(ELevelType::Loading);
        desc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_LOADING;


        desc.textureIndex = selectedBackgroundIndex;

        desc.zOrder = 0.5f;

        _loadingBackground = static_pointer_cast<Background>(
            GAME->Add_UI(
                Protocol::OBJECT_TYPE_BACKGROUND,
                EUILayer::Overlay,
                &desc));

        CHECK_NULL(_loadingBackground, E_FAIL);
    }

    // 오른쪽 하단 회전
    {
        UI_LoadingSpinner::FLoadingSpinnerDesc desc{};
        desc.posX = viewport.x - 180.f;
        desc.posY = viewport.y - 100.f;
        desc.sizeX = 72.f;
        desc.sizeY = 72.f;
        desc.levelIndex = ETOI(ELevelType::Loading);
        desc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_LOADING;
        desc.textureIndex = ETOI(ELoadingTexture::SpinnerLogo);
        desc.zOrder = 0.6f;
        desc.rotationSpeed = XMConvertToRadians(180.f);

        _loadingSpinner = static_pointer_cast<UI_LoadingSpinner>(
            GAME->Add_UI(
                Protocol::OBJECT_TYPE_UI_LOADING_SPINNER,
                EUILayer::Overlay,
                &desc));
        CHECK_NULL(_loadingSpinner, E_FAIL);
    }

    // 하단 중앙 로딩 바
    {
        UI_LoadingProgressBar::FLoadingProgressBarDesc desc{};
        desc.posX = viewport.x * 0.5f;
        desc.posY = viewport.y - 93.f;
        desc.sizeX = 128.f * 8.f;
        desc.sizeY = 8.f * 2.f;
        desc.levelIndex = ETOI(ELevelType::Loading);
        desc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_LOADING;
        desc.textureIndex = ETOI(ELoadingTexture::ProgressBar);
        desc.zOrder = 0.6f;

        _loadingProgressBar = static_pointer_cast<UI_LoadingProgressBar>(
            GAME->Add_UI(
                Protocol::OBJECT_TYPE_UI_LOADING_PROGRESS_BAR,
                EUILayer::Overlay,
                &desc));
        CHECK_NULL(_loadingProgressBar, E_FAIL);
    }

    return S_OK;
}

shared_ptr<Level_Loading> Level_Loading::Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
    ELevelType nextLevelID, bool loadSharedResources, EGameplaySpawnMode gameplaySpawnMode)
{
    auto instance = make_shared<Level_Loading>(device, context);

    if (FAILED(instance->Initialize(nextLevelID, loadSharedResources, gameplaySpawnMode)))
    {
        MSG_BOX("Failed to Create : Level_Loading");
        return nullptr;
    }

    return instance;
}

void Level_Loading::Free()
{
    _loader.reset();

    Level::Free();
}
