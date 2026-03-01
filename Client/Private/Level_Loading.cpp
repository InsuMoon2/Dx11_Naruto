#include "pch.h"
#include "Level_Loading.h"

#include "Background.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_Gameplay.h"
#include "Level_MainTitle.h"

Level_Loading::Level_Loading(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

Level_Loading::~Level_Loading()
{
}

HRESULT Level_Loading::Initialize(ELevelType nextLevelID)
{
    _nextLevelID = nextLevelID;

    if (FAILED(Ready_Layer_UI(TEXT("Layer_UI"))))
        return E_FAIL;

    // 로더 생성
    _loader = Loader::Create(_device, _context, nextLevelID);
    CHECK_NULL(_loader, E_FAIL);

    return S_OK;
}

void Level_Loading::Update(float timeDelta)
{
    if (_loader->IsFinished())// && GetKeyState(VK_RETURN))
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

    {
        Background::FBackgroundDesc desc{};
        desc.name = TEXT("Loading Screen");
        desc.posX = viewport.x * 0.5f;
        desc.posY = viewport.y * 0.5f;
        desc.sizeX = viewport.x;
        desc.sizeY = viewport.y;

        desc.levelIndex = ETOI(ELevelType::Static);
        desc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_LOADING;
        desc.textureIndex = ETOI(ELoadingTexture::MainLoading);

        desc.zOrder = 0.5f;

        CHECK_FAILED(GAME->Add_GameObject(
            ETOI(ELevelType::Static),            // <-- 1. 여기서 견본(Prototype)을 찾아와!
            Protocol::OBJECT_TYPE_BACKGROUND,    // <-- 2. Background 모형을!
            ETOI(ELevelType::Loading),           // <-- 3. 지금 현재 띄울 여기 화면(Loading)에 복제해라!
            uiTag,                               // <-- 4. 레이어 이름
            &desc),                              // <-- 5. 설정값
            E_FAIL);
    }

    return S_OK;
}

shared_ptr<Level_Loading> Level_Loading::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, ELevelType nextLevelID)
{
    auto instance = make_shared<Level_Loading>(device, context);

    if (FAILED(instance->Initialize(nextLevelID)))
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
