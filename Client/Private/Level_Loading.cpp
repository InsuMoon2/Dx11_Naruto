#include "pch.h"
#include "Level_Loading.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_Gameplay.h"
#include "Level_Logo.h"

Level_Loading::Level_Loading(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

Level_Loading::~Level_Loading()
{
}

HRESULT Level_Loading::Initialize(LevelType nextLevelID)
{
    _nextLevelID = nextLevelID;

    if (FAILED(Ready_Layer_Background(TEXT("Layer_Background"))))
        return E_FAIL;

    if (FAILED(Ready_Layer_UI(TEXT("Layer_UI"))))
        return E_FAIL;

    // 로더 생성
    _loader = Loader::Create(_device, _context, nextLevelID);
    NULL_CHECK_RETURN(_loader, E_FAIL);



    return S_OK;
}

void Level_Loading::Update(float timeDelta)
{
    if (_loader->IsFinished() && GetKeyState(VK_RETURN))
    {
        shared_ptr<Level> nextLevel = { nullptr };

        switch (_nextLevelID)
        {
        case LevelType::Logo:
            nextLevel = Level_Logo::Create(_device, _context);
            break;

        case LevelType::GamePlay:
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

HRESULT Level_Loading::Ready_Layer_Background(const wstring& layerTag)
{

    return S_OK;
}

HRESULT Level_Loading::Ready_Layer_UI(const wstring& uiTag)
{

    return S_OK;
}

shared_ptr<Level_Loading> Level_Loading::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, LevelType nextLevelID)
{
    auto instance = make_shared<Level_Loading>(device, context);

    if (FAILED(instance->Initialize(nextLevelID)))
    {
        MSG_BOX("Faield to Created : LevelType Loading");

        return nullptr;
    }

    return instance;
}

void Level_Loading::Free()
{
    _loader.reset();

    Level::Free();
}
