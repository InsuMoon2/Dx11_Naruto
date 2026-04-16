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
#include "Particle_Point.h"
#include "Player.h"
#include "UI_PlayerHUD.h"
#include "Event_Manager.h"
#include "StaticMeshActor.h"
#include "Layer.h"

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

    CHECK_FAILED(Ready_Lights(), E_FAIL);
    CHECK_FAILED(Ready_Layer_Camera(TEXT("Layer_Camera")), E_FAIL);
    CHECK_FAILED(Ready_UI(), E_FAIL);

    {
        CHECK_FAILED(Ready_GroundColliison(), E_FAIL);
        CHECK_FAILED(Rebuild_WallCollisionFromPlacedMeshes(), E_FAIL);
        CHECK_FAILED(Rebuild_ExtraGroundCollisionFromPlacedMeshes(), E_FAIL);
    }
    

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
    lightDesc.ambient  = Vec4(1.f, 1.f, 1.f, 1.f);
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
        desc.position = Vec3(0.f, 15.f, 0.f);
        desc.spawnIndex = 0;

        CHECK_FAILED(GAME->Add_GameObject(levelIndex, Protocol::OBJECT_TYPE_PLAYER_START, layerTag, &desc), E_FAIL);
    }

    return S_OK;
}

HRESULT Level_Konoha::Ready_Layer_GameObject(const wstring& layerTag)
{
    

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

HRESULT Level_Konoha::Append_CollisionInstancesFromDirectory(
    const string& dirPath,
    bool treatAsWall)
{
    if (!fs::exists(dirPath))
        return S_OK;

    const Matrix preTransform = Build_CollisionModelPreTransform();

    for (const auto& entry : fs::directory_iterator(dirPath))
    {
        if (entry.path().extension() != ".meshbin")
            continue;

        const string fullPath = entry.path().string();
        const string fileName = entry.path().filename().string();

        Shared<Model> colModel = Model::Create(
            _device,
            _context,
            EMeshVertexType::StaticMesh,
            fullPath,
            preTransform,
            true);

        if (!colModel)
            continue;

        MovementComponent::FCollisionModelInstance instance{};
        instance.model = colModel;
        instance.worldMatrix = Matrix::Identity;
        instance.hasWorldBounds =
            Try_BuildWorldBoundsFromModel(colModel, instance.worldMatrix, instance.worldBounds);

        const bool isWallLike =
            treatAsWall ||
            fileName.find("Wall") != string::npos ||
            fileName.find("WALL") != string::npos;

        if (isWallLike)
            _wallCollisionModels.push_back(instance);
        else
            _groundCollisionModels.push_back(instance);
    }

    return S_OK;
}

bool Level_Konoha::Is_WallCollisionLayerTag(const wstring& layerTag)
{
    return layerTag == TEXT("Layer_Terrain") || layerTag == TEXT("Layer_Props");
}

bool Level_Konoha::Is_WallCollisionNameCandidate(const string& candidateName)
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

bool Level_Konoha::Is_WallCollisionSizeCandidate(const BoundingBox& bounds)
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

HRESULT Level_Konoha::Rebuild_WallCollisionFromPlacedMeshes()
{
    _wallCollisionModels.clear();

    vector<wstring> layerTags =
    {
        TEXT("Layer_Terrain"),
        TEXT("Layer_Props")
    };

    CHECK_FAILED(Collect_WallCollisionCandidatesFromLayers(layerTags), E_FAIL);

    LOG_INFO("[Level_Konoha] Wall collision rebuilt from placed meshes = {}",
        _wallCollisionModels.size());

    return S_OK;
}

HRESULT Level_Konoha::Append_WallCollisionInstanceFromActor(Shared<StaticMeshActor> actor)
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

    MovementComponent::FCollisionModelInstance collision{};
    collision.model = collisionModel;
    collision.worldMatrix = transform->Get_WorldMatrix();
    collision.hasWorldBounds =
        Try_BuildWorldBoundsFromModel(collisionModel, collision.worldMatrix, collision.worldBounds);

    if (!collision.hasWorldBounds)
        return S_OK;

    if (!Is_WallCollisionSizeCandidate(collision.worldBounds))
        return S_OK;

    _wallCollisionModels.push_back(collision);

    return S_OK;
}

Shared<Model> Level_Konoha::Get_OrCreateWallCollisionModel(const string& modelGuid, const string& resolvedPath)
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

HRESULT Level_Konoha::Collect_WallCollisionCandidatesFromLayers(const vector<wstring>& layerTags)
{
    const uint32 levelIndex = ETOI(ELevelType::Konoha);
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

            CHECK_FAILED(Append_WallCollisionInstanceFromActor(actor), E_FAIL);
        }
    }

    return S_OK;
}

bool Level_Konoha::Is_ExtraGroundNameCandidate(const string& candidateName)
{
    const string lowered = Utils::ToLowerCopy(candidateName);

    static const vector<string> includeKeywords =
    {
        "roof",
        "top",
        "terrace",
        "balcony",
        "platform"
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


HRESULT Level_Konoha::Rebuild_ExtraGroundCollisionFromPlacedMeshes()
{
    _extraGroundCollisionModels.clear();

    const vector<wstring> layerTags =
    {
        TEXT("Layer_Terrain"),
        TEXT("Layer_Props")
    };

    const uint32 levelIndex = ETOI(ELevelType::Konoha);
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

            CHECK_FAILED(Append_ExtraGroundCollisionFromActor(actor), E_FAIL);
        }
    }

    LOG_INFO("[Level_Konoha] Extra ground collision rebuilt from placed meshes = {}",
        _extraGroundCollisionModels.size());

    return S_OK;
}

HRESULT Level_Konoha::Append_ExtraGroundCollisionFromActor(Shared<StaticMeshActor> actor)
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

    if (!Is_ExtraGroundNameCandidate(candidateName))
        return S_OK;

    Shared<Model> collisionModel = Get_OrCreateWallCollisionModel(modelGuid, resolvedPath);
    if (!collisionModel)
        return S_OK;

    MovementComponent::FCollisionModelInstance collision{};
    collision.model = collisionModel;
    collision.worldMatrix = transform->Get_WorldMatrix();
    collision.hasWorldBounds =
        Try_BuildWorldBoundsFromModel(collisionModel, collision.worldMatrix, collision.worldBounds);

    if (!collision.hasWorldBounds)
        return S_OK;

    const float width = collision.worldBounds.Extents.x * 2.f;
    const float depth = collision.worldBounds.Extents.z * 2.f;

    if (max(width, depth) < 1.5f)
        return S_OK;

    _extraGroundCollisionModels.push_back(collision);
    return S_OK;
}


void Level_Konoha::Draw_StaticMeshRender()
{
    // 플레이어 위치 기준으로 짤라보기
    if (INPUT->KeyDown(KEY_TYPE::F3))
    {
        _showCollisionDebug = !_showCollisionDebug;
    }

    if (_showCollisionDebug)
    {
        const uint32 levelIndex = ETOI(ELevelType::Konoha);

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

        for (const auto& collision : _wallCollisionModels)
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
            desc.style.color = Color(1.f, 0.5f, 0.f, 1.f);

            desc.style.duration = 0.f;

            GAME->Draw_DebugMesh(desc);

            ++wallDrawCount;
            if (wallDrawCount >= maxWallDrawCount)
                break;
        }
    }
}

HRESULT Level_Konoha::Ready_GroundColliison()
{
    const string collisionRoot = "../../Client/Bin/Resources/StaticMesh/KonohaVillage02/Meshes/";

    _groundCollisionModels.clear();


    _wallCollisionModels.clear();

    CHECK_FAILED(Append_CollisionInstancesFromDirectory(
        collisionRoot + "Ground_Collision",
        false), E_FAIL);

    //CHECK_FAILED(Append_CollisionInstancesFromDirectory(
    //    collisionRoot + "AreaBorder",
    //    true), E_FAIL);

    LOG_INFO("[Level_Konoha] Ground COL = {}, Wall COL = {}",
        _groundCollisionModels.size(),
        _wallCollisionModels.size());

    return S_OK;
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

    vector<MovementComponent::FCollisionModelInstance> combinedGroundModels = _groundCollisionModels;
    combinedGroundModels.insert(
        combinedGroundModels.end(),
        _extraGroundCollisionModels.begin(),
        _extraGroundCollisionModels.end());

    if (!combinedGroundModels.empty())
    {
        moveCom->Set_GroundCollisionModels(combinedGroundModels);
    }

    if (!_wallCollisionModels.empty())
    {
        moveCom->Set_WallCollisionModels(_wallCollisionModels);
    }
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

