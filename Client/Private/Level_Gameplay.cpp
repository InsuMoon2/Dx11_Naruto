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
#include "Event_Manager.h"
#include "Layer.h"
#include "Mesh.h"
#include "CollisionProxyActor.h"

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

    CHECK_FAILED(Rebuild_CollisionProxyCache(), E_FAIL);

    CHECK_FAILED(Ready_Layer_PlayerStart(TEXT("Layer_PlayerStart")), E_FAIL);

    // 서버에서는 레벨 Load할 때 생긴 몬스터 제거 하고 서버에서 S_AddObject 패킷으로 생성 후 제어
    if (_spawnMode == EGameplaySpawnMode::Server)
    {
        Remove_LocalMonsters_ForServerMode();
    }

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

    if (INPUT->KeyDown(KEY_TYPE::KEY_4))
    {
        Request_EnterKonoha();
    }

}

void Level_Gameplay::Late_Update(float timeDelta)
{
    Level::Late_Update(timeDelta);

    Draw_StaticMeshRender();
}

HRESULT Level_Gameplay::Render()
{
    #ifdef _DEBUG
    //SetWindowText(g_hWnd, TEXT("현재 레벨 : GamePlay"));

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

    return S_OK;
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

Matrix Level_Gameplay::Build_CollisionModelPreTransform()
{
    Matrix scaleMatrix = Matrix::CreateScale(1.f);
    Matrix rotationMatrix = Matrix::CreateRotationY(XMConvertToRadians(180.f));

    return scaleMatrix * rotationMatrix;
}

HRESULT Level_Gameplay::Rebuild_CollisionProxyCache()
{
    _walkableProxyModels.clear();
    _wallProxyModels.clear();
    _worldBlockProxyModels.clear();

    CHECK_FAILED(Collect_CollisionProxyActorsFromLayer(TEXT("Layer_CollisionProxy")), E_FAIL);

    LOG_INFO("[Level_Gameplay] Walkable Proxy = {}", _walkableProxyModels.size());
    LOG_INFO("[Level_Gameplay] Wall Proxy = {}", _wallProxyModels.size());
    LOG_INFO("[Level_Gameplay] WorldBlock Proxy = {}", _worldBlockProxyModels.size());

    return S_OK;
}

HRESULT Level_Gameplay::Collect_CollisionProxyActorsFromLayer(const wstring& layerTag)
{
    const uint32 levelIndex = ETOI(ELevelType::GamePlay);
    const auto& layers = GAME->Get_Layers(levelIndex);

    auto iter = layers.find(layerTag);
    if (iter == layers.end() || !iter->second)
        return S_OK;

    const auto& objects = iter->second->Get_GameObjects();
    for (const auto& obj : objects)
    {
        auto proxyActor = dynamic_pointer_cast<CollisionProxyActor>(obj);
        if (!proxyActor)
            continue;

        CHECK_FAILED(Append_CollisionProxyInstance(proxyActor), E_FAIL);
    }

    return S_OK;
}

HRESULT Level_Gameplay::Append_CollisionProxyInstance(Shared<CollisionProxyActor> actor)
{
    if (!actor || !actor->Is_Enabled())
        return S_OK;

    auto transform = actor->Get_Transform();
    if (!transform)
        return S_OK;

    MovementComponent::FCollisionModelInstance instance{};
    instance.worldMatrix = transform->Get_WorldMatrix();

    auto model = actor->Get_Model();
    if (!model)
        return S_OK;

    instance.model = model;
    instance.hasWorldBounds =
        Try_BuildWorldBoundsFromModel(model, instance.worldMatrix, instance.worldBounds);

    switch (actor->Get_ProxyType())
    {
    case ECollisionProxyType::Walkable:
        _walkableProxyModels.push_back(instance);
        break;

    case ECollisionProxyType::WallRun:
        _wallProxyModels.push_back(instance);
        break;

    case ECollisionProxyType::WorldBlock:
        _worldBlockProxyModels.push_back(instance);
        break;

    default:
        break;
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

    if (!_walkableProxyModels.empty())
    {
        moveCom->Set_GroundCollisionModels(_walkableProxyModels);
    }

    if (!_wallProxyModels.empty())
    {
        moveCom->Set_WallCollisionModels(_wallProxyModels);
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

void Level_Gameplay::Remove_LocalMonsters_ForServerMode()
{
     auto gameObjects = GAME->Get_GameObjects(ETOI(ELevelType::GamePlay));

    for (const auto& obj : gameObjects)
    {
        if (!obj)
            continue;

        if (obj->Get_ObjectType() != Protocol::OBJECT_TYPE_MONSTER)
            continue;

        EVENT->Publish(FEvent_Object::Create(EEventType::Delete_Object, obj));
    }
}

void Level_Gameplay::Request_EnterKonoha()
{
    if (_konohaTransitionRequested)
        return;

    _konohaTransitionRequested = true;

    const EGameplaySpawnMode spawnMode =
        (_spawnMode == EGameplaySpawnMode::Server)
            ? EGameplaySpawnMode::Server
            : EGameplaySpawnMode::LocalOnly;

    GAME->Change_Level(
        ETOI(ELevelType::Loading),
        Level_Loading::Create(_device, _context, ELevelType::Konoha, true, spawnMode));
}

bool Level_Gameplay::Try_BuildWorldBoundsFromModel(Shared<Model> model, const Matrix& worldMatrix,
    BoundingBox& outBounds)
{
    if (!model)
        return false;

    Vec3 minPos = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec3 maxPos = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    bool hasAnyVertex = false;

    for (const auto& mesh : model->Get_Meshes())
    {
        if (!mesh)
            continue;

        const auto& positions = mesh->Get_CPUPositions();
        if (positions.empty())
            continue;

        for (const auto& localPos : positions)
        {
            const Vec3 worldPos = Vec3::Transform(localPos, worldMatrix);

            minPos.x = min(minPos.x, worldPos.x);
            minPos.y = min(minPos.y, worldPos.y);
            minPos.z = min(minPos.z, worldPos.z);

            maxPos.x = max(maxPos.x, worldPos.x);
            maxPos.y = max(maxPos.y, worldPos.y);
            maxPos.z = max(maxPos.z, worldPos.z);

            hasAnyVertex = true;
        }
    }

    if (!hasAnyVertex)
        return false;

    outBounds.Center = (minPos + maxPos) * 0.5f;
    outBounds.Extents = (maxPos - minPos) * 0.5f;

    return true;
}

void Level_Gameplay::Draw_StaticMeshRender()
{
    // 플레이어 위치 기준으로 짤라보기
    if (INPUT->KeyDown(KEY_TYPE::F3))
    {
        _showCollisionDebug = !_showCollisionDebug;
    }

    if (_showCollisionDebug)
    {
        const uint32 levelIndex = ETOI(ELevelType::GamePlay);

        Vec3 debugCenter = Vec3::Zero;
        bool hasDebugCenter = false;

        auto gameObjects = GAME->Get_GameObjects(levelIndex);
        for (const auto& obj : gameObjects)
        {
            if (!obj)
                continue;

            if (obj->Get_ObjectType() != Protocol::OBJECT_TYPE_PLAYER)
                continue;

            auto transform = obj->Get_Transform();
            if (!transform)
                continue;

            debugCenter = transform->Get_WorldPosition();
            hasDebugCenter = true;
            break;
        }

        if (!hasDebugCenter)
        {
            auto activeCamera = GAME->Get_ActiveCamera();
            if (activeCamera && activeCamera->Get_Transform())
            {
                debugCenter = activeCamera->Get_Transform()->Get_WorldPosition();
                hasDebugCenter = true;
            }
        }

        const float debugRadius = 35.f;
        const float debugRadiusSq = debugRadius * debugRadius;

        int32 groundDrawCount = 0;
        int32 wallDrawCount = 0;

        const int32 maxGroundDrawCount = 80;
        const int32 maxWallDrawCount = 80;
    }
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

