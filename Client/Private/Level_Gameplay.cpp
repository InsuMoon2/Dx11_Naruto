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
    CHECK_FAILED(Ready_Layer_GameObject(TEXT("Layer_GameObject")), E_FAIL);
    CHECK_FAILED(Ready_Layer_TempLayer(TEXT("Layer_TempLayer")), E_FAIL);


    return S_OK;
}

void Level_Gameplay::Update(float timeDelta)
{
    Level::Update(timeDelta);

}

void Level_Gameplay::Late_Update(float timeDelta)
{
    Level::Late_Update(timeDelta);

    if (INPUT->KeyDown(KEY_TYPE::KEY_1))
    {
        LOG_INFO("Info Test");
    }

    if (INPUT->KeyDown(KEY_TYPE::KEY_2))
    {
        LOG_WARN("Warning Test");
    }

    if (INPUT->KeyDown(KEY_TYPE::KEY_3))
    {
        LOG_ERROR("Error Test");
    }
}

HRESULT Level_Gameplay::Render()
{
    #ifdef _DEBUG
    SetWindowText(g_hWnd, TEXT("현재 레벨 : GamePlay"));
    #endif
    
    return S_OK;
}

HRESULT Level_Gameplay::Ready_Layer_GameObject(const wstring& layerTag)
{
    CHECK_FAILED(GAME->Add_GameObject(ETOI(ELevelType::GamePlay), Protocol::OBJECT_TYPE_PLAYER, layerTag), E_FAIL);

    return S_OK;
}

HRESULT Level_Gameplay::Ready_Layer_TempLayer(const wstring& layerTag)
{
    CHECK_FAILED(GAME->Add_GameObject(ETOI(ELevelType::GamePlay), Protocol::OBJECT_TYPE_PLAYER, layerTag), E_FAIL);

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

