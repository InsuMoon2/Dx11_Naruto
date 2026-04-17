#include "pch.h"
#include "Level_Konoha.h"
#include "Model.h"
#include "Camera_Free.h"
#include "Mesh.h"
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
#include "Player.h"
#include "UI_PlayerHUD.h"
#include "Event_Manager.h"
#include "Layer.h"
#include "CollisionProxyActor.h"
#include "SkySphereActor.h"

Level_Konoha::Level_Konoha(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

Level_Konoha::~Level_Konoha()
{
    
}

HRESULT Level_Konoha::Initialize(EGameplaySpawnMode spawnMode)
{
    _spawnMode = spawnMode;

    CHECK_FAILED(Ready_Layer_SkySphere(), E_FAIL);

    CHECK_FAILED(Ready_Lights(), E_FAIL);
    CHECK_FAILED(Ready_Layer_Camera(TEXT("Layer_Camera")), E_FAIL);
    CHECK_FAILED(Ready_UI(), E_FAIL);

    CHECK_FAILED(Ready_DefaultGroundCollision(), E_FAIL);
    CHECK_FAILED(Rebuild_CollisionProxyCache(), E_FAIL);

    CHECK_FAILED(Ready_Layer_PlayerStart(TEXT("Layer_PlayerStart")), E_FAIL);
    //CHECK_FAILED(Ready_Effect(), E_FAIL);

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

void Level_Konoha::Update(float timeDelta)
{
    Level::Update(timeDelta);

    if (_spawnMode == EGameplaySpawnMode::Server && !_enterGameSent)
    {
        Try_SendEnterGamePacket();
    }
}

void Level_Konoha::Late_Update(float timeDelta)
{
    Level::Late_Update(timeDelta);

    Draw_StaticMeshRender();

}

HRESULT Level_Konoha::Render()
{
    #ifdef _DEBUG
    //SetWindowText(g_hWnd, TEXT("현재 레벨 : Konoha"));


    #endif
    
    return S_OK;
}

Matrix Level_Konoha::Build_CollisionModelPreTransform()
{
    Matrix scaleMatrix = Matrix::CreateScale(1.f);
    Matrix rotationMatrix = Matrix::CreateRotationY(XMConvertToRadians(180.f));

    return scaleMatrix * rotationMatrix;
}

bool Level_Konoha::Try_BuildWorldBoundsFromModel(
    Shared<Model> model,
    const Matrix& worldMatrix,
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

HRESULT Level_Konoha::Ready_Lights()
{
    FLightDesc lightDesc{};

    lightDesc.type = ELightType::Directional;
    lightDesc.direction  = Vec4(1.f, -1.f, 1.f, 0.f);
    lightDesc.diffuse  = Vec4(1.f, 1.f, 1.f, 1.f);
    lightDesc.ambient  = Vec4(0.18f, 0.18f, 0.18f, 1.f);
    lightDesc.specular = Vec4(1.f, 1.f, 1.f, 1.f);

    CHECK_FAILED(GAME->Add_Light(lightDesc), E_FAIL);

    return S_OK; 
}

HRESULT Level_Konoha::Ready_Layer_Camera(const wstring& layerTag)
{
    const uint32 levelIndex = ETOI(ELevelType::Konoha);

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

        CHECK_FAILED(GAME->Add_GameObject(levelIndex,
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

        CHECK_FAILED(GAME->Add_GameObject(levelIndex,
            Protocol::OBJECT_TYPE_CAMERA_TARGET, layerTag, &desc), E_FAIL);
    }

    return S_OK;
}

HRESULT Level_Konoha::Ready_Layer_PlayerStart(const wstring& layerTag)
{
    const uint32 levelIndex = ETOI(ELevelType::Konoha);

    // TODO : Spawn Point Save&Load로 위치 세팅
    {
        PlayerStart::FPlayerStartDesc desc;
        desc.position = Vec3(0.f, 20.3f, -12.2f);
        desc.spawnIndex = 0;

        CHECK_FAILED(GAME->Add_GameObject(levelIndex, Protocol::OBJECT_TYPE_PLAYER_START, layerTag, &desc), E_FAIL);
    }

    return S_OK;
}

HRESULT Level_Konoha::Ready_Layer_GameObject(const wstring& layerTag)
{
    

    return S_OK;
}

HRESULT Level_Konoha::Ready_Layer_SkySphere()
{
    const uint32 levelIndex = ETOI(ELevelType::Konoha);
    const wstring& layerTag = TEXT("Layer_SkySphere");

    {
        SkySphereActor::FSkySphereDesc desc{};
        desc.name = TEXT("Sky.Base");
        desc.modelComponentName = "Sky_SkySphere_Base";
        desc.followCamera = true;
        desc.useBlend = false;
        desc.twoSided = true;
        desc.renderStyle = 1;
        desc.uvTiling = Vec2(1.f, 1.f);
        desc.uvScrollSpeed = Vec2::Zero;
        desc.colorTint = Vec4(1.f, 1.f, 1.f, 1.f);
        desc.horizonColor = Vec4(0.34111f, 0.569243f, 1.f, 1.f);
        desc.zenithColor = Vec4(0.069653f, 0.295485f, 0.56f, 1.f);
        desc.opacity = 1.f;
        desc.emissiveStrength = 1.f;
        desc.scale = Vec3(1.f, 1.f, 1.f);

        CHECK_FAILED(GAME->Add_GameObject(levelIndex, Protocol::OBJECT_TYPE_SKY_SPHERE, layerTag, &desc), E_FAIL);
    }

    {
        SkySphereActor::FSkySphereDesc desc{};
        desc.name = TEXT("Sky.Cloud.Main");
        desc.modelComponentName = "Sky_CloudPlate_01";
        desc.followCamera = true;
        desc.useBlend = true;
        desc.twoSided = true;
        desc.renderStyle = 2;
        desc.pitch = -8.f;
        desc.yaw = 18.f;
        desc.roll = 6.f;
        desc.uvTiling = Vec2(1.f, 1.f);
        desc.uvScrollSpeed = Vec2::Zero;
        desc.colorTint = Vec4(1.f, 1.f, 1.f, 1.f);
        desc.subUVTiling = Vec2(1.f, 1.f);
        desc.subUVScrollSpeed = Vec2::Zero;
        desc.opacity = 0.32f;
        desc.emissiveStrength = 1.f;
        desc.scale = Vec3(1.f, 1.f, 1.f);

        CHECK_FAILED(GAME->Add_GameObject(levelIndex, Protocol::OBJECT_TYPE_SKY_SPHERE, layerTag, &desc), E_FAIL);
    }

  /*  {
        SkySphereActor::FSkySphereDesc desc{};
        desc.name = TEXT("Sky.Cloud.FillA");
        desc.modelComponentName = "Sky_CloudPlate_01";
        desc.followCamera = true;
        desc.useBlend = true;
        desc.twoSided = true;
        desc.renderStyle = 2;
        desc.pitch = 5.f;
        desc.yaw = 128.f;
        desc.roll = -14.f;
        desc.uvTiling = Vec2(1.15f, 1.15f);
        desc.uvScrollSpeed = Vec2::Zero;
        desc.colorTint = Vec4(1.f, 1.f, 1.f, 0.92f);
        desc.subUVTiling = Vec2(1.f, 1.f);
        desc.subUVScrollSpeed = Vec2::Zero;
        desc.opacity = 0.22f;
        desc.emissiveStrength = 1.f;
        desc.scale = Vec3(1.f, 1.f, 1.f);

        CHECK_FAILED(GAME->Add_GameObject(levelIndex, Protocol::OBJECT_TYPE_SKY_SPHERE, layerTag, &desc), E_FAIL);
    }

    {
        SkySphereActor::FSkySphereDesc desc{};
        desc.name = TEXT("Sky.Cloud.FillB");
        desc.modelComponentName = "Sky_CloudPlate_01";
        desc.followCamera = true;
        desc.useBlend = true;
        desc.twoSided = true;
        desc.renderStyle = 2;
        desc.pitch = -2.f;
        desc.yaw = 242.f;
        desc.roll = 18.f;
        desc.uvTiling = Vec2(0.9f, 0.9f);
        desc.uvScrollSpeed = Vec2::Zero;
        desc.colorTint = Vec4(1.f, 1.f, 1.f, 0.88f);
        desc.subUVTiling = Vec2(1.f, 1.f);
        desc.subUVScrollSpeed = Vec2::Zero;
        desc.opacity = 0.16f;
        desc.emissiveStrength = 1.f;
        desc.scale = Vec3(1.f, 1.f, 1.f);

        CHECK_FAILED(GAME->Add_GameObject(levelIndex, Protocol::OBJECT_TYPE_SKY_SPHERE, layerTag, &desc), E_FAIL);
    }*/


    return S_OK;
}

HRESULT Level_Konoha::Ready_UI()
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
        this, &Level_Konoha::On_PlayerObjectSpawned);

    return S_OK;
}

void Level_Konoha::Build_CollisionProxyEntries(vector<FProxyEntry>& outEntries) const
{
    outEntries.clear();
    outEntries.reserve(
        _defaultGroundModels.size() +
        _walkableProxyModels.size() +
        _wallProxyModels.size() +
        _worldBlockProxyModels.size());

    const auto appendEntries =
        [&outEntries](const vector<MovementComponent::FCollisionModelInstance>& instances, ECollisionProxyType proxyType)
        {
             for (const auto& instance : instances)
             {
                 FProxyEntry entry{};
                 entry.instance = instance;
                 entry.proxyType = static_cast<uint8>(proxyType);
                 outEntries.push_back(entry);
             }
        };

    appendEntries(_defaultGroundModels, ECollisionProxyType::Walkable);
    appendEntries(_walkableProxyModels, ECollisionProxyType::Walkable);
    appendEntries(_wallProxyModels, ECollisionProxyType::WallRun);
    appendEntries(_worldBlockProxyModels, ECollisionProxyType::WorldBlock);
}

HRESULT Level_Konoha::Ready_DefaultGroundCollision()
{
    _defaultGroundModels.clear();

    const fs::path groundDirectory =
        L"../../Client/Bin/Resources/StaticMesh/KonohaVillage02/Meshes/Ground_Collision";

    CHECK_FAILED(Append_CollisionInstancesFromDirectory(groundDirectory, _defaultGroundModels), E_FAIL);

    LOG_INFO("[Level_Konoha] Default Ground = {}", _defaultGroundModels.size());

    return S_OK;
}

HRESULT Level_Konoha::Append_CollisionInstancesFromDirectory(
    const fs::path& directoryPath,
    vector<MovementComponent::FCollisionModelInstance>& outInstances)
{
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath))
    {
        LOG_ERROR("[Level_Konoha] Ground collision directory not found: {}", directoryPath.string());
        return E_FAIL;
    }

    const Matrix preTransform = Build_CollisionModelPreTransform();

    for (const auto& entry : fs::directory_iterator(directoryPath))
    {
        if (!entry.is_regular_file())
            continue;

        const fs::path meshPath = entry.path();
        if (Utils::ToLowerCopy(meshPath.extension().string()) != ".meshbin")
            continue;

        auto model = Model::Create(
            _device,
            _context,
            EMeshVertexType::StaticMesh,
            meshPath.string(),
            preTransform,
            true);

        if (!model)
        {
            LOG_WARN("[Level_Konoha] Failed to load default ground mesh: {}", meshPath.string());
            continue;
        }

        MovementComponent::FCollisionModelInstance instance{};
        instance.model = model;
        instance.worldMatrix = Matrix::Identity;
        instance.hasWorldBounds = Try_BuildWorldBoundsFromModel(
            model,
            instance.worldMatrix,
            instance.worldBounds);

        if (!instance.hasWorldBounds)
        {
            LOG_WARN("[Level_Konoha] Failed to build bounds for default ground mesh: {}", meshPath.string());
            continue;
        }

        outInstances.push_back(instance);
    }

    return S_OK;
}

HRESULT Level_Konoha::Rebuild_CollisionProxyCache()
{
    _walkableProxyModels.clear();
    _wallProxyModels.clear();
    _worldBlockProxyModels.clear();

    CHECK_FAILED(Collect_CollisionProxyActorsFromLayer(TEXT("Layer_CollisionProxy")), E_FAIL);

    vector<FProxyEntry> proxyEntries;
    Build_CollisionProxyEntries(proxyEntries);

    GAME->Ready_CollisionProxy(proxyEntries);

    LOG_INFO("[Level_Konoha] Walkable Proxy = {}", _walkableProxyModels.size());
    LOG_INFO("[Level_Konoha] Wall Proxy = {}", _wallProxyModels.size());
    LOG_INFO("[Level_Konoha] WorldBlock Proxy = {}", _worldBlockProxyModels.size());

    return S_OK;
}

HRESULT Level_Konoha::Collect_CollisionProxyActorsFromLayer(const wstring& layerTag)
{
    const uint32 levelIndex = ETOI(ELevelType::Konoha);
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

HRESULT Level_Konoha::Append_CollisionProxyInstance(Shared<CollisionProxyActor> actor)
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
    instance.hasWorldBounds = Try_BuildWorldBoundsFromModel(model, instance.worldMatrix, instance.worldBounds);

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

void Level_Konoha::Draw_StaticMeshRender()
{
    if (INPUT->KeyDown(KEY_TYPE::F3))
    {
        _showCollisionDebug = !_showCollisionDebug;
    }

    if (!_showCollisionDebug)
        return;

    const uint32 levelIndex = ETOI(ELevelType::Konoha);

    Vec3 debugCenter = Vec3::Zero;
    bool hasDebugCenter = false;

    const auto gameObjects = GAME->Get_GameObjects(levelIndex);
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

    if (!hasDebugCenter)
        return;

    const float debugRadius = 40.f;
    const float debugRadiusSq = debugRadius * debugRadius;

    const int32 maxDefaultGroundDrawCount = 120;
    const int32 maxWalkableDrawCount = 80;
    const int32 maxWallDrawCount = 80;
    const int32 maxWorldBlockDrawCount = 80;

    int32 defaultGroundDrawCount = 0;
    int32 walkableDrawCount = 0;
    int32 wallDrawCount = 0;
    int32 worldBlockDrawCount = 0;

    const auto drawProxyBoxes =
        [debugCenter, debugRadiusSq](
            const vector<MovementComponent::FCollisionModelInstance>& instances,
            const Color& color,
            int32 maxDrawCount,
            int32& drawCount)
        {
            for (const auto& instance : instances)
            {
                if (drawCount >= maxDrawCount)
                    break;

                if (!instance.hasWorldBounds)
                    continue;

                const Vec3 toCenter = instance.worldBounds.Center - debugCenter;
                if (toCenter.LengthSquared() > debugRadiusSq)
                    continue;

                FDebugBoxDesc debugBoxDesc{};
                debugBoxDesc.center = instance.worldBounds.Center;
                debugBoxDesc.extents = instance.worldBounds.Extents;
                debugBoxDesc.rotation = Quat::Identity;
                debugBoxDesc.style.color = color;
                debugBoxDesc.style.duration = 0.f;
                debugBoxDesc.style.depthEnabled = true;

                GAME->Draw_DebugBox(debugBoxDesc);
                ++drawCount;
            }
        };

    drawProxyBoxes(
        _defaultGroundModels,
        Color(1.f, 0.f, 0.2f, 1.f),
        maxDefaultGroundDrawCount,
        defaultGroundDrawCount);

    drawProxyBoxes(
        _walkableProxyModels,
        Color(0.f, 1.f, 0.f, 1.f),
        maxWalkableDrawCount,
        walkableDrawCount);

    drawProxyBoxes(
        _wallProxyModels,
        Color(1.f, 0.5f, 0.f, 1.f),
        maxWallDrawCount,
        wallDrawCount);

    drawProxyBoxes(
        _worldBlockProxyModels,
        Color(0.f, 0.6f, 1.f, 1.f),
        maxWorldBlockDrawCount,
        worldBlockDrawCount);

    FDebugSphereDesc debugSphereDesc{};
    debugSphereDesc.center = debugCenter;
    debugSphereDesc.radius = 0.3f;
    debugSphereDesc.style.color = Color(1.f, 1.f, 0.f, 1.f);
    debugSphereDesc.style.duration = 0.f;
    debugSphereDesc.style.depthEnabled = false;

    GAME->Draw_DebugSphere(debugSphereDesc);
}

void Level_Konoha::Spawn_LocalPlayer()
{
    const uint32 levelIndex = ETOI(ELevelType::Konoha);

    auto gameObjects = GAME->Get_GameObjects(levelIndex);

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
        .AtLevel(levelIndex)
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

void Level_Konoha::On_PlayerObjectSpawned(Shared<GameObject> obj)
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

    vector<MovementComponent::FCollisionModelInstance> combinedGroundModels = _defaultGroundModels;
    combinedGroundModels.insert(
        combinedGroundModels.end(),
        _walkableProxyModels.begin(),
        _walkableProxyModels.end());

    moveCom->Set_GroundCollisionModels(combinedGroundModels);
    moveCom->Set_WallCollisionModels(_wallProxyModels);
}

void Level_Konoha::Try_SendEnterGamePacket()
{
    if (_enterGameSent)
        return;

    if (!NetworkManager::GetInstance()->IsConnected())
        return;

    const uint32 levelIndex = ETOI(ELevelType::Konoha);

    Vec3 spawnPos = Vec3(0.f, 0.f, 0.f);
    float spawnRotY = 0.f;

    auto gameObjects = GAME->Get_GameObjects(levelIndex);
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

void Level_Konoha::Remove_LocalMonsters_ForServerMode()
{
    const uint32 levelIndex = ETOI(ELevelType::Konoha);

    auto gameObjects = GAME->Get_GameObjects(levelIndex);

    for (const auto& obj : gameObjects)
    {
        if (!obj)
            continue;

        if (obj->Get_ObjectType() != Protocol::OBJECT_TYPE_MONSTER)
            continue;

        EVENT->Publish(FEvent_Object::Create(EEventType::Delete_Object, obj));
    }
}

shared_ptr<Level_Konoha> Level_Konoha::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EGameplaySpawnMode spawnMode)
{
    auto instance = make_shared<Level_Konoha>(device, context);

    if (FAILED(instance->Initialize(spawnMode)))
    {
        MSG_BOX("Failed to Create : Level_Konoha");

        return nullptr;
    }

    return instance;
}

void Level_Konoha::Free()
{
    if (_playerObjectSpawnedHandle.IsValid())
    {
        GAME->Get_DelegateHub().OnPlayerObjectSpawned.Remove(_playerObjectSpawnedHandle);
        _playerObjectSpawnedHandle.Reset();
    }

    Level::Free();

}

