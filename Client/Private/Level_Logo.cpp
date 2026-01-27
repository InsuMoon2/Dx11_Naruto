#include "pch.h"
#include "Level_Logo.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_Loading.h"

Level_Logo::Level_Logo(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

Level_Logo::~Level_Logo()
{
}

HRESULT Level_Logo::Initialize()
{


    return S_OK;
}

void Level_Logo::Update(float timeDelta)
{
    Level::Update(timeDelta);

    if (GetKeyState(VK_SPACE) & 0x8000)
    {
        GAME->Change_Level(ETOI(LevelType::Loading), Level_Loading::Create(_device, _context, LevelType::GamePlay));
    }
}

void Level_Logo::Late_Update(float timeDelta)
{
    Level::Late_Update(timeDelta);

}

HRESULT Level_Logo::Render()
{
    #ifdef _DEBUG
    SetWindowText(g_hWnd, TEXT("현재 레벨 : Logo"));
    #endif
    

    return S_OK;
}

shared_ptr<Level_Logo> Level_Logo::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Level_Logo>(device, context);

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Create : Level_Logo");

        return nullptr;
    }

    return instance;
}

void Level_Logo::Free()
{
    Level::Free();

}

