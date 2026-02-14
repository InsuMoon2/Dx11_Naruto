#include "pch.h"
#include "Level_Gameplay.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "Spawn_Helper.h"

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

#pragma region Builder 적용 전
    //json overrides;
    //overrides["scale"] = { 5.f, 5.f, 5.f };
    //auto player = GAME->Instantiate_Prefab("TestPlayer", overrides);
    //if (player)
    //{
    //    GAME->Add_GameObject(ETOI(ELevelType::GamePlay), TEXT("Layer_GameObject"), player);
    //}
#pragma endregion

    auto player = Spawn_Helper::Prefab("TestPlayer")
        .AtLevel(ETOI(ELevelType::GamePlay))
        .InLayer(TEXT("Layer_Builder"))
        .Position({ 0.f, 0.f, -5.f })
        .Scale({ 1.f, 1.f, 1.f })
        .Spawn();


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

