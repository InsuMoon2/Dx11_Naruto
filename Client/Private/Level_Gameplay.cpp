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
#include "StaticMeshActor.h"
#include "Layer.h"
#include "Mesh.h"

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

    {
        CHECK_FAILED(Ready_GroundColliison(), E_FAIL);
        CHECK_FAILED(Rebuild_WallCollisionFromPlacedMeshes(), E_FAIL);
    }

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

HRESULT Level_Gameplay::Ready_GroundColliison()
{
    const string colDir = "../../Client/Bin/Resources/StaticMesh/ExamStadium/Meshes/";

    if (!fs::exists(colDir))
        return S_OK;

    const Matrix preTransform = Build_CollisionModelPreTransform();

    _groundCollisionModels.clear();
    _wallCollisionProxies.clear();

    for (const auto& entry : fs::recursive_directory_iterator(colDir))
    {
        if (entry.path().extension() != ".meshbin")
            continue;

        const string fullPath = entry.path().string();
        const string fileName = entry.path().filename().string();

        const bool isGroundLike =
            fileName.find("Ground") != string::npos ||
            fileName.find("ground") != string::npos ||
            fileName.find("Terrain") != string::npos ||
            fileName.find("terrain") != string::npos ||
            fileName.find("AreaBorder") != string::npos ||
            fileName.find("_COL") != string::npos;

        if (!isGroundLike)
            continue;

        Shared<Model> colModel = Model::Create(
            _device,
            _context,
            EMeshVertexType::StaticMesh,
            fullPath,
            preTransform,
            true);

        if (!colModel)
            continue;

        MovementComponent::FCollisionModelInstance collision{};
        collision.model = colModel;
        collision.worldMatrix = Matrix::Identity;
        collision.hasWorldBounds =
            Try_BuildWorldBoundsFromModel(colModel, collision.worldMatrix, collision.worldBounds);

        _groundCollisionModels.push_back(collision);
    }

    LOG_INFO("[Level_Gameplay] Ground COL = {}, Wall COL = {}",
        _groundCollisionModels.size(),
        _wallCollisionProxies.size());

    return S_OK;
}



Matrix Level_Gameplay::Build_CollisionModelPreTransform()
{
    Matrix scaleMatrix = Matrix::CreateScale(1.f);
    Matrix rotationMatrix = Matrix::CreateRotationY(XMConvertToRadians(180.f));

    return scaleMatrix * rotationMatrix;
}

bool Level_Gameplay::Try_BuildWallProxyFromActorBounds(
    const BoundingBox& localBounds,
    const Matrix& worldMatrix,
    MovementComponent::FWallCollisionProxy& outProxy)
{
    Vec3 scale = Vec3::One;
    Quat rotation = Quat::Identity;
    Vec3 translation = Vec3::Zero;

    Matrix tempMat = worldMatrix;
    if (!tempMat.Decompose(scale, rotation, translation))
        return false;

    const Vec3 absScale(fabsf(scale.x), fabsf(scale.y), fabsf(scale.z));
    Vec3 extents = localBounds.Extents * absScale;
    extents *= 1.10f;

    extents.x = max(extents.x, 0.50f);
    extents.y = max(extents.y, 0.90f);
    extents.z = max(extents.z, 0.50f);

    XMFLOAT4 orientation(rotation.x, rotation.y, rotation.z, rotation.w);

    BoundingOrientedBox worldObb(
        Vec3::Transform(localBounds.Center, worldMatrix),
        extents,
        orientation);

    XMFLOAT3 corners[BoundingOrientedBox::CORNER_COUNT]{};
    worldObb.GetCorners(corners);

    Vec3 minPos(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec3 maxPos(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (const auto& corner : corners)
    {
        minPos.x = min(minPos.x, corner.x);
        minPos.y = min(minPos.y, corner.y);
        minPos.z = min(minPos.z, corner.z);

        maxPos.x = max(maxPos.x, corner.x);
        maxPos.y = max(maxPos.y, corner.y);
        maxPos.z = max(maxPos.z, corner.z);
    }

    outProxy.worldObb = worldObb;
    outProxy.worldBounds.Center = (minPos + maxPos) * 0.5f;
    outProxy.worldBounds.Extents = (maxPos - minPos) * 0.5f;
    outProxy.hasWorldBounds = true;

    return true;
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

    if (!_wallCollisionProxies.empty())
    {
        moveCom->Set_WallCollisionProxies(_wallCollisionProxies);
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

bool Level_Gameplay::Is_WallCollisionLayerTag(const wstring& layerTag)
{
    return layerTag == TEXT("Layer_Terrain") || layerTag == TEXT("Layer_Props");
}

bool Level_Gameplay::Is_WallCollisionNameCandidate(const string& candidateName)
{
    const string lowered = Utils::ToLowerCopy(candidateName);

    static const vector<string> includeKeywords =
    {
        "building",
        "roof",
        "wall",
        "slope",
        "house",
        "shop",
        "tower"
    };

    static const vector<string> excludeKeywords =
    {
        "curtain",
        "lantern",
        "banner",
        "tree",
        "bush",
        "grass",
        "plant"
    };

    for (const auto& keyword : excludeKeywords)
    {
        if (lowered.find(keyword) != string::npos)
            return false;
    }

    for (const auto& keyword : includeKeywords)
    {
        if (lowered.find(keyword) != string::npos)
            return true;
    }

    return false;
}

bool Level_Gameplay::Is_WallCollisionSizeCandidate(const BoundingBox& bounds)
{
    const float width = bounds.Extents.x * 2.f;
    const float height = bounds.Extents.y * 2.f;
    const float depth = bounds.Extents.z * 2.f;

    // 너무 작은애들은 제거
    if (height < 1.5f)
        return false;

    if (max(width, depth) < 2.0f)
        return false;

    return true;
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

HRESULT Level_Gameplay::Rebuild_WallCollisionFromPlacedMeshes()
{
    _wallCollisionProxies.clear();

    vector<wstring> layerTags =
    {
        TEXT("Layer_Terrain"),
        TEXT("Layer_Props")
    };

    CHECK_FAILED(Collect_WallCollisionCandidatesFromLayers(layerTags), E_FAIL);

    LOG_INFO("[Level_GamePlay] Wall collision rebuilt from placed meshes = {}",
        _wallCollisionProxies.size());

    return S_OK;
}

HRESULT Level_Gameplay::Collect_WallCollisionCandidatesFromLayers(const vector<wstring>& layerTags)
{
    const uint32 levelIndex = ETOI(ELevelType::GamePlay);
    const auto layers = GAME->Get_Layers(levelIndex);

    for (const auto& layerTag : layerTags)
    {
        auto iter = layers.find(layerTag);
        if (iter == layers.end() || !iter->second)
            continue;

        const auto& objects = iter->second->Get_GameObjects();
        for (const auto& obj : objects)
        {
            if (!obj)
                continue;

            if (obj->Get_ObjectType() != Protocol::OBJECT_TYPE_STATIC_MESH)
                continue;

            auto actor = dynamic_pointer_cast<StaticMeshActor>(obj);
            if (!actor)
                continue;

            CHECK_FAILED(Append_WallCollisionProxyFromActor(actor), E_FAIL);
        }
    }

    return S_OK;
}

HRESULT Level_Gameplay::Append_WallCollisionProxyFromActor(Shared<StaticMeshActor> actor)
{
    if (!actor)
        return S_OK;

    auto transform = actor->Get_Transform();
    if (!transform)
        return S_OK;

    const string& modelGuid = actor->Get_ModelGuid();
    if (modelGuid.empty())
        return S_OK;

    string resolvedPath = actor->Get_ResolvedPath();
    if (resolvedPath.empty())
    {
        resolvedPath = Utils::ToString(GAME->Resolve_AssetPath(modelGuid));
    }

    const string candidateName =
        Utils::ToString(actor->Get_Name()) + "|" + resolvedPath + "|" + modelGuid;

    if (!Is_WallCollisionNameCandidate(candidateName))
        return S_OK;

    Shared<Model> collisionModel = Get_OrCreateWallCollisionModel(modelGuid, resolvedPath);
    if (!collisionModel)
        return S_OK;

    BoundingBox localBounds{};
    if (!Try_BuildWorldBoundsFromModel(collisionModel, Matrix::Identity, localBounds))
        return S_OK;

    MovementComponent::FWallCollisionProxy proxy{};
    if (!Try_BuildWallProxyFromActorBounds(localBounds, transform->Get_WorldMatrix(), proxy))
        return S_OK;

    if (!Is_WallCollisionSizeCandidate(proxy.worldBounds))
        return S_OK;

    proxy.refineModel = collisionModel;
    proxy.refineWorldMatrix = transform->Get_WorldMatrix();
    proxy.hasRefineModel = true;
    proxy.debugName = candidateName;

    _wallCollisionProxies.push_back(proxy);

    return S_OK;
}

Shared<Model> Level_Gameplay::Get_OrCreateWallCollisionModel(const string& modelGuid, const string& resolvedPath)
{
    auto iter = _wallCollisionModelCache.find(modelGuid);
    if (iter != _wallCollisionModelCache.end())
        return iter->second;

    if (resolvedPath.empty())
        return nullptr;

    const Matrix preTransform = Build_CollisionModelPreTransform();

    // wall collision용 모델은 CPU raycast가 가능해야 함
    Shared<Model> collisionModel = Model::Create(
        _device,
        _context,
        EMeshVertexType::StaticMesh,
        resolvedPath,
        preTransform,
        true);

    if (!collisionModel)
        return nullptr;

    _wallCollisionModelCache.emplace(modelGuid, collisionModel);
    return collisionModel;
}

void Level_Gameplay::Draw_StaticMeshRender()
{
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

        for (const auto& collision : _groundCollisionModels)
        {
            if (!collision.model)
                continue;

            if (hasDebugCenter && collision.hasWorldBounds)
            {
                const Vec3 toCenter = collision.worldBounds.Center - debugCenter;
                if (toCenter.LengthSquared() > debugRadiusSq)
                    continue;
            }

            FDebugMeshDesc desc{};
            desc.model = collision.model;
            desc.worldMatrix = collision.worldMatrix;
            desc.style.color = Color(0.f, 0.5f, 1.f, 1.f);

            // 10초로하니까, 큐에 10초동안남아서 렌더 프레임이 더 떨어졌음.
            // 어차피 Render에서 계속 호출하니까 duration을 0으로 해도 됨
            desc.style.duration = 0.f;

            GAME->Draw_DebugMesh(desc);

            ++groundDrawCount;
            if (groundDrawCount >= maxGroundDrawCount)
                break;
        }

        for (const auto& proxy : _wallCollisionProxies)
        {
            if (!proxy.hasWorldBounds)
                continue;

            if (hasDebugCenter)
            {
                const Vec3 toCenter = proxy.worldBounds.Center - debugCenter;
                if (toCenter.LengthSquared() > debugRadiusSq)
                    continue;
            }

            Draw_WallCollisionProxyDebug(proxy);

            ++wallDrawCount;
            if (wallDrawCount >= maxWallDrawCount)
                break;
        }
    }
}

void Level_Gameplay::Draw_WallCollisionProxyDebug(const MovementComponent::FWallCollisionProxy& proxy)
{
    FDebugBoxDesc desc{};
    desc.center = proxy.worldObb.Center;
    desc.extents = proxy.worldObb.Extents;
    desc.rotation = Quat(
        proxy.worldObb.Orientation.x,
        proxy.worldObb.Orientation.y,
        proxy.worldObb.Orientation.z,
        proxy.worldObb.Orientation.w);
    desc.style.color = Color(1.f, 0.5f, 0.f, 1.f);
    desc.style.duration = 0.f;

    GAME->Draw_DebugBox(desc);
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

