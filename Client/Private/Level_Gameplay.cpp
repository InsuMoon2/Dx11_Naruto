#include "pch.h"
#include "Level_Gameplay.h"
#include "Model.h"
#include "Camera_Free.h"
#include "Camera_Target.h"
#include "Client_PacketHandler.h"
#include "Customizer_Manager.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "MovementComponent.h"
#include "NetworkManager.h"
#include "Spawn_Helper.h"
#include "PlayerStart.h"
#include "GameInstance.h"
#include "Debug_Manager.h"
#include "Particle_Point.h"
#include "Player.h"
#include "UI_PlayerHUD.h"

Level_Gameplay::Level_Gameplay(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

Level_Gameplay::~Level_Gameplay()
{
}

HRESULT Level_Gameplay::Initialize(EGameplaySpawnMode spawnMode)
{
    _spawnMode = spawnMode;

    CHECK_FAILED(Ready_Lights(), E_FAIL);
    CHECK_FAILED(Ready_Layer_Camera(TEXT("Layer_Camera")), E_FAIL);
    CHECK_FAILED(Ready_UI(), E_FAIL);
    CHECK_FAILED(Ready_GroundColliison(), E_FAIL);

    CHECK_FAILED(Ready_Layer_PlayerStart(TEXT("Layer_PlayerStart")), E_FAIL);
    //CHECK_FAILED(Ready_Effect(), E_FAIL);

    if (_spawnMode == EGameplaySpawnMode::LocalOnly)
    {
        Spawn_LocalPlayer();

        _enterGameSent = true;
    }
    else
    {
        _enterGameSent = false;

        Try_SendEnterGamePacket();
    }


   

    return S_OK;
}

void Level_Gameplay::Update(float timeDelta)
{
    Level::Update(timeDelta);

    if (_spawnMode == EGameplaySpawnMode::Server && !_enterGameSent)
    {
        Try_SendEnterGamePacket();
    }

    if (INPUT->KeyDown(KEY_TYPE::KEY_3))
    {
        GAME->Play_Cinematic(TEXT("Test1"));
    }

    

}

void Level_Gameplay::Late_Update(float timeDelta)
{
    Level::Late_Update(timeDelta);


}

HRESULT Level_Gameplay::Render()
{
    #ifdef _DEBUG
    SetWindowText(g_hWnd, TEXT("현재 레벨 : GamePlay"));

    if (INPUT->KeyDown(KEY_TYPE::F3))
    {
        for (const auto& colModel : _groundCollisionModels)
        {
            FDebugMeshDesc desc{};
            desc.model = colModel;

            //desc.style.depthEnabled = false;
            desc.worldMatrix = Matrix::CreateTranslation(0.f, 0.05f, 0.f);
            desc.style.color = Color(0.f, 0.5f, 1.f, 1.f); // 파란 테두리로
            desc.style.duration = 10.0f;
            GAME->Draw_DebugMesh(desc);
        }

        for (const auto& colModel : _wallCollisionModels)
        {
            FDebugMeshDesc desc{};
            desc.model = colModel;

            //desc.style.depthEnabled = false;
            desc.worldMatrix = Matrix::CreateTranslation(0.f, 0.05f, 0.f);
            desc.style.color = Color(1.f, 0.5f, 0.f, 1.f); // 빨간 테두리로
            desc.style.duration = 10.0f;
            GAME->Draw_DebugMesh(desc);
        }
    }

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

    CHECK_FAILED(
        GAME->Add_GameObject(
            ETOI(ELevelType::GamePlay),
            Protocol::OBJECT_TYPE_TERRAIN,
            layerTag),
        E_FAIL);

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

        _playerHUD = static_pointer_cast<UI_PlayerHUD>(
            GAME->Add_UI(
                Protocol::OBJECT_TYPE_UI_PLAYER_HUD,
                EUILayer::HUD,
                &desc));

        CHECK_NULL(_playerHUD, E_FAIL);
    }

    _playerObjectSpawnedHandle = GAME->Get_DelegateHub().OnPlayerObjectSpawned.Add(
        this, &Level_Gameplay::On_PlayerObjectSpawned);

    return S_OK;
}

HRESULT Level_Gameplay::Ready_Effect()
{
    Particle_Point::FParticlePointDesc snowDesc{};
    snowDesc.name = L"Particle_Snow";
    snowDesc.position = Vec3(64.f, 30.f, 64.f);
    snowDesc.shaderType = Protocol::COMPONENT_TYPE_SHADER_PARTICLE_POINT;
    snowDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_PARTICLE_SNOW;
    snowDesc.textureIndex = 0;
    snowDesc.bufferDesc.numInstances = 5000;
    snowDesc.bufferDesc.center = Vec3::Zero;
    snowDesc.bufferDesc.range = Vec3(129.f, 1.f, 129.f);
    snowDesc.bufferDesc.scale = Vec2(0.2f, 0.5f);
    snowDesc.bufferDesc.speed = Vec2(3.f, 7.f);
    snowDesc.bufferDesc.lifeTime = Vec2(3.f, 5.f);
    snowDesc.bufferDesc.isLoop = true;
    snowDesc.bufferDesc.moveMode = VIBuffer_Particle_Point::EMoveMode::Drop;

    CHECK_FAILED(
        GAME->Add_GameObject(
            ETOI(ELevelType::GamePlay),
            Protocol::OBJECT_TYPE_INSTANCED_PARTICLE_POINT,
            TEXT("Layer_Effect"),
            &snowDesc),
        E_FAIL);

    return S_OK;
}

HRESULT Level_Gameplay::Ready_GroundColliison()
{
    string colDir = "../../Client/Bin/Resources/StaticMesh/ExamStadium/Meshes/";

    if (fs::exists(colDir))
    {
        for (const auto& entry : fs::directory_iterator(colDir))
        {
            if (entry.path().extension() == ".meshbin")
            {
                string filename = entry.path().filename().string();

                if (filename.find("_COL") != string::npos)
                {
                    Matrix preTransform = Matrix::CreateScale(0.01f);

                    Shared<Model> colModel = Model::Create(_device, _context,
                        EMeshVertexType::StaticMesh,
                        entry.path().string(),
                        /*preTransform*/Matrix::Identity,
                        true);

                    if (!colModel)
                        continue;

                    // 파일명에 Wall이 들어가면 wall collision으로 분리한다.
                    if (filename.find("Wall") != string::npos || filename.find("WALL") != string::npos)
                        _wallCollisionModels.push_back(colModel);
                    else
                        _groundCollisionModels.push_back(colModel);
                }
            }
        }

        LOG_INFO(
            "[Level GamePlay] Loaded Ground COL = {}, Wall COL = {}",
            _groundCollisionModels.size(),
            _wallCollisionModels.size());
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

    auto playerObj = Spawn_Helper::Prefab("TestPlayer2")
        .AtLevel(ETOI(ELevelType::GamePlay))
        .Position(spawnPos)
        .InLayer(TEXT("Layer_Player"))
        .Spawn();

    auto player = dynamic_pointer_cast<Player>(playerObj);
    CHECK_NULL(player);

    auto custom = GET_SINGLE(Customizer_Manager);
    const auto& customDesc = custom->Get_CustomizerDesc();

    for (int32 i = 0; i < ETOI(ContainerObject::EPartSlot::END); ++i)
    {
        const wstring& partTag = customDesc.partTags[i];
        if (!partTag.empty())
        {
            player->Apply_CustomizingPart(static_cast<ContainerObject::EPartSlot>(i), partTag);
        }
    }

    // Camera Target 세팅
    GAME->Get_DelegateHub().OnPlayerSpawned.Broadcast(player->Get_Component<Transform>());
    // 플레이어
    GAME->Get_DelegateHub().OnPlayerObjectSpawned.Broadcast(player);
}

void Level_Gameplay::On_PlayerObjectSpawned(Shared<GameObject> obj)
{
    if (!_playerHUD || !obj)
        return;

    auto player = dynamic_pointer_cast<Player>(obj);
    if (!player)
        return;

    _playerHUD->Bind_Player(player);

    auto moveCom = obj->Get_Component<MovementComponent>();
    if (!moveCom)
        return;

    if (!_groundCollisionModels.empty())
    {
        moveCom->Set_GroundCollisionModels(_groundCollisionModels);
    }

    if (!_wallCollisionModels.empty())
    {
        moveCom->Set_WallCollisionModels(_wallCollisionModels);
    }

}

void Level_Gameplay::Try_SendEnterGamePacket()
{
    if (_enterGameSent)
        return;

    if (!NetworkManager::GetInstance()->IsConnected())
        return;

    Vec3 spawnPos = Vec3(0.f, 0.f, 0.f);
    float spawnRotY = 0.f;

    auto gameObjects = GAME->Get_GameObjects(ETOI(ELevelType::GamePlay));
    for (auto& obj : gameObjects)
    {
        if (obj->Get_ObjectType() == Protocol::OBJECT_TYPE_PLAYER_START)
        {
            auto transform = obj->Get_Component<Transform>();
            if (transform)
            {
                spawnPos = transform->Get_WorldPosition();
                spawnRotY = transform->Get_LocalRotation().ToEuler().y;
            }
            break;
        }
    }

    auto buf = Client_PacketHandler::Make_C_EnterGame(spawnPos, spawnRotY);
    if (!buf)
        return;

    NetworkManager::GetInstance()->Send_Packet(buf);
    _enterGameSent = true;
}

shared_ptr<Level_Gameplay> Level_Gameplay::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EGameplaySpawnMode spawnMode)
{
    auto instance = make_shared<Level_Gameplay>(device, context);

    if (FAILED(instance->Initialize(spawnMode)))
    {
        MSG_BOX("Failed to Create : Level_GamePlay");

        return nullptr;
    }

    return instance;
}

void Level_Gameplay::Free()
{
    if (_playerObjectSpawnedHandle.IsValid())
    {
        GAME->Get_DelegateHub().OnPlayerObjectSpawned.Remove(_playerObjectSpawnedHandle);
        _playerObjectSpawnedHandle.Reset();
    }

    Level::Free();

}

