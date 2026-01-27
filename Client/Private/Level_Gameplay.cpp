#include "pch.h"
#include "Level_Gameplay.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_Loading.h"

Level_Gameplay::Level_Gameplay(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

Level_Gameplay::~Level_Gameplay()
{
}

HRESULT Level_Gameplay::Initialize()
{

    return S_OK;
}

void Level_Gameplay::Update(float timeDelta)
{
    Level::Update(timeDelta);

}

void Level_Gameplay::Late_Update(float timeDelta)
{
    Level::Late_Update(timeDelta);

}

HRESULT Level_Gameplay::Render()
{
    #ifdef _DEBUG
    SetWindowText(g_hWnd, TEXT("현재 레벨 : GamePlay"));
    #endif
    

    return S_OK;
}

shared_ptr<Level_Gameplay> Level_Gameplay::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Level_Gameplay>(device, context);

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Create : Level_GamePlay");

        return nullptr;
    }

    return instance;
}

void Level_Gameplay::Free()
{
    Level::Free();

}

