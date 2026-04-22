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
#include "WaveTrigger.h"

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

    // ExamStadium은 별도 proxy 레이어가 없어도 기본 바닥 collision이 항상 준비되어야 한다.
    CHECK_FAILED(Ready_DefaultGroundCollision(), E_FAIL);
    CHECK_FAILED(Rebuild_CollisionProxyCache(), E_FAIL);

    CHECK_FAILED(Ready_Layer_PlayerStart(TEXT("Layer_PlayerStart")), E_FAIL);

    // 서버에서는 레벨 Load할 때 생긴 몬스터 제거 하고 서버에서 S_AddObject 패킷으로 생성 후 제어
    if (_spawnMode == EGameplaySpawnMode::Server)
    {
        Remove_LocalMonsters_ForServerMode();
        Disable_LocalWaveTriggers_ForServerMode();
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

    _konohaTransitionRequested = false;

    _waveStartedHandle = GAME->Get_DelegateHub().OnWaveStarted.Add(this, &Level_Gameplay::On_WaveStarted);
    _waveClearedHandle = GAME->Get_DelegateHub().OnWaveCleared.Add(this, &Level_Gameplay::On_WaveCleared);

    return S_OK;
}

void Level_Gameplay::Update(float timeDelta)
{
    Level::Update(timeDelta);

    if (_konohaTransitionRequested)
    {
        const EGameplaySpawnMode spawnMode =
            (_spawnMode == EGameplaySpawnMode::Server)
            ? EGameplaySpawnMode::Server
            : EGameplaySpawnMode::LocalOnly;

        // Gameplay 진입 시 shared resource는 이미 로드되어 있으므로,
        // Konoha 복귀에서는 맵 오브젝트만 다시 구성하고 공용 테이블은 재큐잉하지 않는다.
        GAME->Change_Level(
            ETOI(ELevelType::Loading),
            Level_Loading::Create(_device, _context, ELevelType::Konoha, false, spawnMode));

        return;
    }

    if (_spawnMode == EGameplaySpawnMode::Server && !_enterGameSent)
    {
        Try_SendEnterGamePacket();
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

void Level_Gameplay::On_WaveCleared(const string& waveTag)
{
    if (waveTag != "GamePlayClearWave")
        return;

    Request_EnterKonoha();
}

void Level_Gameplay::On_WaveStarted(const string& waveTag)
{
    if (waveTag != "GamePlayClearWave")
        return;

    GAME->Play_Cinematic(L"GamePlayClearWave");
}

HRESULT Level_Gameplay::Ready_Lights()
{
    FLightDesc lightDesc{};

    lightDesc.type = ELightType::Directional;
    lightDesc.direction = Vec4(1.f, -1.f, 1.f, 0.f);
    lightDesc.diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
    lightDesc.ambient = Vec4(0.18f, 0.18f, 0.18f, 1.f);
    lightDesc.specular = Vec4(1.f, 1.f, 1.f, 1.f);

    CHECK_FAILED(GAME->Add_Light(lightDesc), E_FAIL);

#if 1
    //FLightDesc pointLightDesc{};
    //pointLightDesc.type = ELightType::Point;
    //pointLightDesc.position = Vec4(5.f, 6.f, -5.f, 1.f);
    //pointLightDesc.range = 20.f;
    //pointLightDesc.diffuse = Vec4(1.f, 0.85f, 0.75f, 1.f);
    //pointLightDesc.ambient = Vec4(0.05f, 0.04f, 0.04f, 1.f);
    //pointLightDesc.specular = Vec4(1.f, 1.f, 1.f, 1.f);
    //
    //CHECK_FAILED(GAME->Add_Light(pointLightDesc), E_FAIL);
#endif

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
        desc.levelIndex = ETOI(ELevelType::GamePlay);

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

void Level_Gameplay::Disable_LocalWaveTriggers_ForServerMode()
{
    const auto gameObjects = GAME->Get_GameObjects(ETOI(ELevelType::GamePlay));

    for (const auto& obj : gameObjects)
    {
        auto waveTrigger = dynamic_pointer_cast<WaveTrigger>(obj);
        if (!waveTrigger)
            continue;

        waveTrigger->Set_ServerAuthoritative(true);
    }
}

HRESULT Level_Gameplay::Ready_DefaultGroundCollision()
{
    _defaultGroundModels.clear();

    const fs::path groundDirectory =
        L"../../Client/Bin/Resources/StaticMesh/ExamStadium/Meshes";

    CHECK_FAILED(Append_CollisionInstancesFromDirectory(groundDirectory, _defaultGroundModels), E_FAIL);

    LOG_INFO("[Level_Gameplay] Default Ground = {}", _defaultGroundModels.size());

    return S_OK;
}

// ExamStadium Meshes 폴더에서 ground collision 용도로 쓸 meshbin만 골라 cache에 추가한다.
HRESULT Level_Gameplay::Append_CollisionInstancesFromDirectory(
    const fs::path& directoryPath,
    vector<MovementComponent::FCollisionModelInstance>& outInstances)
{
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath))
    {
        LOG_ERROR("[Level_Gameplay] Ground collision directory not found: {}", directoryPath.string());
        return E_FAIL;
    }

    const Matrix preTransform = Build_CollisionModelPreTransform();
    vector<fs::path> collisionGroundMeshes;
    vector<fs::path> fallbackGroundMeshes;

    for (const auto& entry : fs::directory_iterator(directoryPath))
    {
        if (!entry.is_regular_file())
            continue;

        const fs::path meshPath = entry.path();
        if (Utils::ToLowerCopy(meshPath.extension().string()) != ".meshbin")
            continue;

        const string fileNameLower = Utils::ToLowerCopy(meshPath.filename().string());
        const bool isGroundMesh = (fileNameLower.find("ground") != string::npos);
        if (!isGroundMesh)
            continue;

        const bool isCollisionMesh = (fileNameLower.find("_col") != string::npos);
        if (isCollisionMesh)
        {
            collisionGroundMeshes.push_back(meshPath);
        }
        else
        {
            fallbackGroundMeshes.push_back(meshPath);
        }
    }

    const vector<fs::path>& targetMeshes =
        !collisionGroundMeshes.empty() ? collisionGroundMeshes : fallbackGroundMeshes;

    if (targetMeshes.empty())
    {
        LOG_ERROR("[Level_Gameplay] No ground collision meshes found in: {}", directoryPath.string());
        return E_FAIL;
    }

    for (const auto& meshPath : targetMeshes)
    {
        auto model = Model::Create(
            _device,
            _context,
            EMeshVertexType::StaticMesh,
            meshPath.string(),
            preTransform,
            true);

        if (!model)
        {
            LOG_WARN("[Level_Gameplay] Failed to load default ground mesh: {}", meshPath.string());
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
            LOG_WARN("[Level_Gameplay] Failed to build bounds for default ground mesh: {}", meshPath.string());
            continue;
        }

        outInstances.push_back(instance);
    }

    if (outInstances.empty())
    {
        LOG_ERROR("[Level_Gameplay] Ground collision meshes were found but no valid bounds were built.");
        return E_FAIL;
    }

    return S_OK;
}

// Gameplay 레벨에서 사용할 collision proxy entry를 Konoha와 같은 방식으로 구성한다.
void Level_Gameplay::Build_CollisionProxyEntries(vector<FProxyEntry>& outEntries) const
{
    outEntries.clear();
    outEntries.reserve(
        _defaultGroundModels.size() +
        _walkableProxyModels.size() +
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
    appendEntries(_worldBlockProxyModels, ECollisionProxyType::WorldBlock);
}

HRESULT Level_Gameplay::Rebuild_CollisionProxyCache()
{
    _walkableProxyModels.clear();
    _wallProxyModels.clear();
    _worldBlockProxyModels.clear();

    CHECK_FAILED(Collect_CollisionProxyActorsFromLayer(TEXT("Layer_CollisionProxy")), E_FAIL);

    vector<FProxyEntry> proxyEntries;
    Build_CollisionProxyEntries(proxyEntries);

    GAME->Ready_CollisionProxy(proxyEntries);

    LOG_INFO("[Level_Gameplay] Default Ground = {}", _defaultGroundModels.size());
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

    vector<MovementComponent::FCollisionModelInstance> combinedGroundModels = _defaultGroundModels;
    combinedGroundModels.insert(
        combinedGroundModels.end(),
        _walkableProxyModels.begin(),
        _walkableProxyModels.end());

    moveCom->Set_GroundCollisionModels(combinedGroundModels);
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

        if (obj->Get_ObjectType() != Protocol::OBJECT_TYPE_MONSTER &&
            obj->Get_ObjectType() != Protocol::OBJECT_TYPE_BOSS_PAIN)
            continue;

        EVENT->Publish(FEvent_Object::Create(EEventType::Delete_Object, obj));
    }
}

void Level_Gameplay::Request_EnterKonoha()
{
    if (_konohaTransitionRequested)
        return;

    _konohaTransitionRequested = true;
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
    if (INPUT->KeyDown(KEY_TYPE::F3))
    {
        _showCollisionDebug = !_showCollisionDebug;
    }

    if (!_showCollisionDebug)
        return;

    const uint32 levelIndex = ETOI(ELevelType::GamePlay);

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

    const float debugRadius = 35.f;
    const float debugRadiusSq = debugRadius * debugRadius;
    const int32 maxDefaultGroundDrawCount = 120;
    const int32 maxWalkableDrawCount = 80;
    const int32 maxWorldBlockDrawCount = 80;

    int32 defaultGroundDrawCount = 0;
    int32 walkableDrawCount = 0;
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

    if (_waveStartedHandle.IsValid())
    {
        GAME->Get_DelegateHub().OnWaveStarted.Remove(_waveStartedHandle);
        _waveStartedHandle.Reset();
    }

    if (_waveClearedHandle.IsValid())
    {
        GAME->Get_DelegateHub().OnWaveCleared.Remove(_waveClearedHandle);
        _waveClearedHandle.Reset();
    }

    Level::Free();

}

