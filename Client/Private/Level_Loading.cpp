#include "pch.h"
#include "Level_Loading.h"

#include <UI_Text.h>

#include "Background.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_Equipment.h"
#include "Level_Gameplay.h"
#include "Level_MainTitle.h"
#include "UI_LoadingSpinner.h"
#include "UI_LoadingProgressBar.h"

Level_Loading::Level_Loading(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

Level_Loading::~Level_Loading()
{
}

HRESULT Level_Loading::Initialize(ELevelType nextLevelID, bool loadSharedResources)
{
    _nextLevelID = nextLevelID;
    _loadSharedResources = loadSharedResources;

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
    const int jobPerFrame = 1;

    for (int i = 0; i < jobPerFrame; ++i)
    {
        FLoadJob job{};
        if (!_loader->Pop_NextJob(job))
            break;

        if (FAILED(_loader->Execute_Job_OnMainThread(job)))
        {
            MSG_BOX("Failed to execute loading job");
            return;
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
            nextLevel = Level_Gameplay::Create(_device, _context);
            break;

        case ELevelType::Equipment:
            nextLevel = Level_Equipment::Create(_device, _context);
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
    Vec2 viewport = Vec2(GAME->Get_WindowWidth(), GAME->Get_WindowHeight());

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
        desc.textureIndex = ETOI(ELoadingTexture::MainLoading);

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

shared_ptr<Level_Loading> Level_Loading::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, ELevelType nextLevelID, bool loadSharedResources)
{
    auto instance = make_shared<Level_Loading>(device, context);

    if (FAILED(instance->Initialize(nextLevelID, loadSharedResources)))
    {
        MSG_BOX("Faield to Created : Loading");

        return nullptr;
    }

    return instance;
}

void Level_Loading::Free()
{
    _loader.reset();

    Level::Free();
}
