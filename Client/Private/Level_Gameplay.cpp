#include "pch.h"
#include "Level_Gameplay.h"

#include "Camera_Free.h"
#include "Camera_Target.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "NetworkManager.h"
#include "Spawn_Helper.h"
#include "PlayerStart.h"

#include "UI_PlayerHUD.h"

Level_Gameplay::Level_Gameplay(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

Level_Gameplay::~Level_Gameplay()
{
}

HRESULT Level_Gameplay::Initialize()
{
    CHECK_FAILED(Ready_Lights(), E_FAIL);

    if (FAILED(Load_LevelFromJson(L"[20260304]MonsterCopy2")))
    {
        // 풀백
        LOG_WARN("Level JSON load failed, using hardcoded setup");
        CHECK_FAILED(Ready_Layer_PlayerStart(TEXT("Layer_PlayerStart")), E_FAIL);
        CHECK_FAILED(Ready_Layer_GameObject(TEXT("Layer_GameObject")), E_FAIL);
    }

    CHECK_FAILED(Ready_Layer_Camera(TEXT("Layer_Camera")), E_FAIL);

    // 서버 연결 없으면 로컬 플레이어 스폰
    if (!NetworkManager::GetInstance()->IsConnected())
    {
        Spawn_LocalPlayer();
    }

    CHECK_FAILED(Ready_UI(), E_FAIL);

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

HRESULT Level_Gameplay::Ready_Lights()
{
    FLightDesc lightDesc{};

    lightDesc.type = ELightType::Directional;
    lightDesc.direction  = Vec4(1.f, -1.f, 1.f, 0.f);
    lightDesc.diffuse  = Vec4(1.f, 1.f, 1.f, 1.f);
    lightDesc.ambient  = Vec4(1.f, 1.f, 1.f, 1.f);
    lightDesc.specular = Vec4(1.f, 1.f, 1.f, 1.f);

    CHECK_FAILED(GAME->Add_Light(lightDesc), E_FAIL);

    return S_OK; 
}

HRESULT Level_Gameplay::Ready_Layer_Camera(const wstring& layerTag)
{
    // Camera Free
    {
        Camera_Free::FCameraFreeDesc desc;
        desc.speedPerSec = 10.f;
        desc.rotationPerSec = 90.f;
        desc.eye = Vec3(0.f, 15.f, -15.f);
        desc.at = Vec3(0.f, 0.f, 0.f);
        desc.fovY = XMConvertToRadians(60.f);
        desc.nearZ = 0.1f;
        desc.farZ = 1000.f;
        desc.scale = Vec3(1.f, 1.f, 1.f);

        desc.mouseSensor = 15.65f; // 마우스 감도

        CHECK_FAILED(GAME->Add_GameObject(ETOI(ELevelType::GamePlay),
            Protocol::OBJECT_TYPE_CAMERA_FREE, layerTag, &desc), E_FAIL);
    }

    // Camera Target
    {
        Camera_Target::FCameraTargetDesc desc;
        desc.speedPerSec = 10.f;
        desc.rotationPerSec = 90.f;
        desc.eye = Vec3(0.f, 10.f, -10.f);
        desc.at = Vec3(0.f, 0.f, 0.f);
        desc.fovY = XMConvertToRadians(60.f);
        desc.nearZ = 0.1f;
        desc.farZ = 1000.f;
        desc.scale = Vec3(1.f, 1.f, 1.f);

        desc.offset = Vec3(0.f, 2.f, -2.f);
        desc.followSpeed = 5.f;

        CHECK_FAILED(GAME->Add_GameObject(ETOI(ELevelType::GamePlay),
            Protocol::OBJECT_TYPE_CAMERA_TARGET, layerTag, &desc), E_FAIL);
    }

    return S_OK;
}

HRESULT Level_Gameplay::Ready_Layer_PlayerStart(const wstring& layerTag)
{
    // TODO : Spawn Point Save&Load로 위치 세팅
    {
        PlayerStart::FPlayerStartDesc desc;
        desc.position = Vec3(0.f, 5.f, 0.f);
        desc.spawnIndex = 0;

        CHECK_FAILED(GAME->Add_GameObject(ETOI(ELevelType::GamePlay), Protocol::OBJECT_TYPE_PLAYER_START, layerTag, &desc), E_FAIL);
    }
}

HRESULT Level_Gameplay::Ready_Layer_GameObject(const wstring& layerTag)
{
    CHECK_FAILED(GAME->Add_GameObject(ETOI(ELevelType::GamePlay), Protocol::OBJECT_TYPE_TERRAIN, L"Layer_Terrain"), E_FAIL);


    auto monster = Spawn_Helper::Prefab("Monster1")
        .AtLevel(ETOI(ELevelType::GamePlay))
        .InLayer(TEXT("Layer_Builder"))
        .Position({ 1.f, 1.f, -5.f })
        .Scale({ 1.5f, 1.5f, 1.5f })
        .Spawn();

    return S_OK;
}

HRESULT Level_Gameplay::Ready_UI()
{
    {
        UIObject::FUIDesc desc;
        desc.posX = 0.f;
        desc.posY = 0.f;
        desc.sizeX = 1.f;
        desc.sizeY = 1.f;
        desc.zOrder = 0.5f;
        desc.levelIndex = ETOI(ELevelType::Static);

        auto playerHUD = UI_PlayerHUD::Create(_device, _context, &desc);

        if (!playerHUD)
            return E_FAIL;

        CHECK_FAILED(GAME->Add_UI_ToLayer(EUILayer::HUD, playerHUD), E_FAIL);
    }

    return S_OK;
}

void Level_Gameplay::Spawn_LocalPlayer()
{
    auto gameObjects = GAME->Get_GameObjects(ETOI(ELevelType::GamePlay));

    Vec3 spawnPos = Vec3(0.f, 0.f, 0.f);

    for (auto& obj : gameObjects)
    {
        if (obj->Get_ObjectType() == Protocol::OBJECT_TYPE_PLAYER_START)
        {
            spawnPos = obj->Get_Component<Transform>()->Get_WorldPosition();
            break;
        }
    }

    auto player = Spawn_Helper::Prefab("TestPlayer")
        .AtLevel(ETOI(ELevelType::GamePlay))
        .Position(spawnPos)
        .InLayer(TEXT("Layer_Builder"))
        .Spawn();

    CHECK_NULL(player);

    // Camera Target 세팅
    GAME->Get_DelegateHub().OnPlayerSpawned.Broadcast(player->Get_Component<Transform>());

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

