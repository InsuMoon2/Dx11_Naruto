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

// 수업코드식 perspective shadow에서는 shadow RT도 viewport 비율을 따르므로 aspect도 화면 비율을 사용한다.
static constexpr bool GAMEPLAY_SHADOW_USE_VIEWPORT_ASPECT = true;
// viewport 비율을 직접 계산하지 못하는 경로에서도 수업 프로젝트의 16:9 기준을 유지하기 위한 fallback aspect다.
static constexpr float GAMEPLAY_SHADOW_RT_ASPECT = 1280.f / 720.f;
// 플레이어 발밑 그림자가 우선이라 shadow target을 상체보다 낮게 둔다.
static constexpr float GAMEPLAY_SHADOW_TARGET_HEIGHT_OFFSET = 3.f;
// Gameplay는 우선 플레이어 기준 shadow 정상화가 목표이므로, target을 앞쪽 지형으로 밀지 않는다.
static constexpr float GAMEPLAY_SHADOW_TARGET_AHEAD_DISTANCE = 0.f;
// Shadow Eye를 너무 멀리 두면 경기장 바깥 산맥이 shadow RT를 먹어버리므로 eye 거리를 더 줄인다.
static constexpr float GAMEPLAY_SHADOW_EYE_DISTANCE = 95.f;
// perspective shadow camera를 수동 디버그할 때 쓰는 fallback FOV다. Gameplay 기본 shadow는 ortho를 사용한다.
static constexpr float GAMEPLAY_SHADOW_FOV_Y = 126.f;
// far가 크면 산맥/외벽이 shadow RT 대부분을 차지하므로 플레이어 주변 바닥 위주로 더 줄인다.
static constexpr float GAMEPLAY_SHADOW_FAR = 220.f;
// near plane을 너무 크게 두면 플레이어 주변 geometry가 통째로 잘려서 RT가 다시 비어 보일 수 있으므로 낮게 유지한다.
static constexpr float GAMEPLAY_SHADOW_NEAR = 0.1f;
// 플레이어 주변 shadow texel 밀도를 유지하기 위한 orthographic coverage다.
static constexpr float GAMEPLAY_SHADOW_ORTHO_WIDTH = 80.f;
// 16:9 shadow RT에서 플레이어 주변 바닥을 충분히 담되 texel이 커지지 않도록 높이를 제한한다.
static constexpr float GAMEPLAY_SHADOW_ORTHO_HEIGHT = 45.f;

// Tutorial/Gameplay Arena 메시들은 저장 데이터 기준 원점 주변에 있으므로 기본 shadow focus를 Area 내부로 고정한다.
static Vec3 Get_GameplayAreaShadowTarget()
{
    return Vec3(0.f, GAMEPLAY_SHADOW_TARGET_HEIGHT_OFFSET, 0.f);
}

// Gameplay 레벨에서 네트워크 입장/로컬 스폰이 같은 PlayerStart를 쓰도록 좌표와 회전을 찾을 때 호출한다.
// 레벨 JSON에서 이미 로드된 PlayerStart가 있으면 그 transform을 그대로 반환하고, 없으면 false를 반환한다.
static bool Try_FindGameplayPlayerStartTransform(uint32 levelIndex, Vec3& outSpawnPos, float& outSpawnRotY)
{
    outSpawnPos = Vec3::Zero;
    outSpawnRotY = 0.f;

    const auto gameObjects = GAME->Get_GameObjects(levelIndex);
    for (const auto& obj : gameObjects)
    {
        if (!obj || obj->Get_ObjectType() != Protocol::OBJECT_TYPE_PLAYER_START)
            continue;

        auto transform = obj->Get_Transform();
        if (!transform)
            continue;

        outSpawnPos = transform->Get_WorldPosition();
        outSpawnRotY = transform->Get_LocalRotation().ToEuler().y;
        return true;
    }

    return false;
}

// 현재 레벨의 플레이어나 활성 카메라에서 shadow camera의 eye/target을 잡기 위한 helper다.
static bool Try_BuildGameplayShadowCameraFromCurrentView(uint32 levelIndex, Vec3& outShadowEye, Vec3& outShadowTarget)
{
    bool hasTarget = false;
    Vec3 playerPosition = Vec3::Zero; // shadow focus의 기준이 되는 플레이어 월드 좌표다.

    const auto gameObjects = GAME->Get_GameObjects(levelIndex);
    for (const auto& obj : gameObjects)
    {
        if (!obj || obj->Get_ObjectType() != Protocol::OBJECT_TYPE_PLAYER)
            continue;

        auto transform = obj->Get_Transform();
        if (!transform)
            continue;

        playerPosition = transform->Get_WorldPosition();
        outShadowTarget = playerPosition + Vec3(0.f, GAMEPLAY_SHADOW_TARGET_HEIGHT_OFFSET, 0.f);
        hasTarget = true;
        break;
    }

    if (hasTarget)
    {
        outShadowTarget = playerPosition + Vec3(0.f, GAMEPLAY_SHADOW_TARGET_HEIGHT_OFFSET, 0.f);
        // 수업코드식 shadow에서는 eye를 광원 방향으로 다시 계산하므로 helper는 플레이어 기준 target만 안정적으로 맞춘다.
        outShadowEye = outShadowTarget + Vec3(0.f, 15.f, -12.f);
        return true;
    }

    // 플레이어가 아직 ObjectManager에 보이지 않아도 카메라 forward 대신 Arena 내부를 본다.
    outShadowTarget = Get_GameplayAreaShadowTarget();
    outShadowEye = outShadowTarget + Vec3(0.f, 15.f, -12.f);
    return true;
}

HRESULT Level_Gameplay::Initialize(EGameplaySpawnMode spawnMode)
{
    // Gameplay에 실제 진입하는 순간 이전 화면에서 재생 중이던 BGM을 정리한다.
    GAME->Stop_SoundChannel(ESoundChannel::BGM, 0.45f);

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
    Update_DynamicShadowLightFromView();

    Update_MissionClearSequence(timeDelta);

    if (INPUT->KeyDown(KEY_TYPE::KEY_0))
        Request_EnterKonoha();

    if (_konohaTransitionRequested)
    {
        const EGameplaySpawnMode spawnMode =
            (_spawnMode == EGameplaySpawnMode::Server)
            ? EGameplaySpawnMode::Server
            : EGameplaySpawnMode::LocalOnly;

        GAME->Change_Level(
            ETOI(ELevelType::Loading),
            Level_Loading::Create(_device, _context, ELevelType::Konoha, false, spawnMode));

        return;
    }

    if (_spawnMode == EGameplaySpawnMode::Server && !_enterGameSent)
    {
        Try_SendEnterGamePacket();
    }

    _collisionModelRefreshAccumulator += timeDelta;
    if (_collisionModelRefreshAccumulator >= 0.1f)
    {
        _collisionModelRefreshAccumulator = 0.f;

        const uint32 levelIndex = ETOI(ELevelType::GamePlay);
        const auto gameObjects = GAME->Get_GameObjects(levelIndex);
        for (const auto& obj : gameObjects)
        {
            if (!obj)
                continue;

            Apply_CollisionModelsToPlayer(obj);
        }
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

HRESULT Level_Gameplay::On_LevelChunkLoaded(const wstring& fileName)
{
    CHECK_FAILED(Rebuild_CollisionProxyCache(), E_FAIL);
    GAME->Invalidate_StaticShadowMap();

    return S_OK;
}

void Level_Gameplay::On_WaveCleared(const string& waveTag)
{
    if (waveTag != "GamePlayClearWave")
        return;

    Start_MissionClearSequence();
}

void Level_Gameplay::On_WaveStarted(const string& waveTag)
{
    if (waveTag != "GamePlayClearWave")
        return;

    GAME->Play_Cinematic(L"GamePlayClearWave");

    if (_playerHUD)
        _playerHUD->Show_MissionTitle(L"적을 쓰러뜨려라!", 2.f);
}

HRESULT Level_Gameplay::Ready_Lights()
{
    // Gameplay 기본 shadow 설정이 Editor Override에 막히지 않도록 레벨 진입 시 override를 초기화한다.
    GAME->Set_EditorPrimaryShadowLightOverrideEnabled(false);

    // 새 레벨 라이트를 등록하기 전에 이전 레벨에서 남은 라이트를 정리한다.
    GAME->Clear_Lights();

    FLightDesc lightDesc{};

    lightDesc.type = ELightType::Directional;
    lightDesc.direction = Vec4(-1.f, -1.f, -1.f, 0.f);
    lightDesc.diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
    lightDesc.ambient = Vec4(0.18f, 0.18f, 0.18f, 1.f);
    lightDesc.specular = Vec4(1.f, 1.f, 1.f, 1.f);

    lightDesc.castShadow = true;
    lightDesc.shadowMapSize = 2048;
    lightDesc.shadowCenter = Get_GameplayAreaShadowTarget();
    lightDesc.shadowOrthoWidth = GAMEPLAY_SHADOW_ORTHO_WIDTH;
    lightDesc.shadowOrthoHeight = GAMEPLAY_SHADOW_ORTHO_HEIGHT;
    lightDesc.useShadowCamera = false;
    lightDesc.shadowTarget = Get_GameplayAreaShadowTarget();
    lightDesc.shadowFovY = XMConvertToRadians(GAMEPLAY_SHADOW_FOV_Y);
    if (GAMEPLAY_SHADOW_USE_VIEWPORT_ASPECT)
        lightDesc.shadowAspect = max(1.f, GAME->Get_ViewportWidth()) / max(1.f, GAME->Get_ViewportHeight());
    else
        lightDesc.shadowAspect = GAMEPLAY_SHADOW_RT_ASPECT;
    lightDesc.shadowNear = GAMEPLAY_SHADOW_NEAR;
    lightDesc.shadowFar = GAMEPLAY_SHADOW_FAR;
    lightDesc.shadowBias = 0.00025f;
    lightDesc.shadowStrength = 0.68f;
    lightDesc.shadowSoftness = 0.35f;

    Vec3 lightDir = Vec3(lightDesc.direction.x, lightDesc.direction.y, lightDesc.direction.z); // 정사광원 방향 기준으로 shadow eye를 고정하기 위한 방향 벡터다.
    if (lightDir.LengthSquared() <= FLT_EPSILON)
        lightDir = Vec3(-1.f, -1.f, -1.f);
    else
        lightDir.Normalize();

    lightDesc.shadowEye = lightDesc.shadowTarget - lightDir * GAMEPLAY_SHADOW_EYE_DISTANCE;

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

void Level_Gameplay::Update_DynamicShadowLightFromView()
{
    FLightDesc lightDesc{};

    lightDesc.type = ELightType::Directional;
    lightDesc.direction = Vec4(-1.f, -1.f, -1.f, 0.f);
    lightDesc.diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
    lightDesc.ambient = Vec4(0.18f, 0.18f, 0.18f, 1.f);
    lightDesc.specular = Vec4(1.f, 1.f, 1.f, 1.f);

    lightDesc.castShadow = true;
    lightDesc.shadowMapSize = 2048;
    lightDesc.shadowCenter = Get_GameplayAreaShadowTarget();
    lightDesc.shadowOrthoWidth = GAMEPLAY_SHADOW_ORTHO_WIDTH;
    lightDesc.shadowOrthoHeight = GAMEPLAY_SHADOW_ORTHO_HEIGHT;
    lightDesc.useShadowCamera = false;
    lightDesc.shadowTarget = Get_GameplayAreaShadowTarget();
    lightDesc.shadowFovY = XMConvertToRadians(GAMEPLAY_SHADOW_FOV_Y);
    if (GAMEPLAY_SHADOW_USE_VIEWPORT_ASPECT)
        lightDesc.shadowAspect = max(1.f, GAME->Get_ViewportWidth()) / max(1.f, GAME->Get_ViewportHeight());
    else
        lightDesc.shadowAspect = GAMEPLAY_SHADOW_RT_ASPECT;
    lightDesc.shadowNear = GAMEPLAY_SHADOW_NEAR;
    lightDesc.shadowFar = GAMEPLAY_SHADOW_FAR;
    lightDesc.shadowBias = 0.00025f;
    lightDesc.shadowStrength = 0.68f;
    lightDesc.shadowSoftness = 0.35f;

    Vec3 dynamicShadowEye = Vec3::Zero;
    Vec3 dynamicShadowTarget = Vec3::Zero;
    if (Try_BuildGameplayShadowCameraFromCurrentView(ETOI(ELevelType::GamePlay), dynamicShadowEye, dynamicShadowTarget))
    {
        lightDesc.shadowTarget = dynamicShadowTarget;
        lightDesc.shadowCenter = dynamicShadowTarget;
    }

    Vec3 lightDir = Vec3(lightDesc.direction.x, lightDesc.direction.y, lightDesc.direction.z); // 수업코드처럼 shadow eye를 directional light 방향 축으로 계산하기 위한 광원 방향 벡터다.
    if (lightDir.LengthSquared() <= FLT_EPSILON)
        lightDir = Vec3(-1.f, -1.f, -1.f);
    else
        lightDir.Normalize();

    // Gameplay도 shadow target만 추적하고, shadow eye는 항상 빛 방향 반대쪽에 고정한다.
    lightDesc.shadowEye = lightDesc.shadowTarget - lightDir * GAMEPLAY_SHADOW_EYE_DISTANCE;

    const HRESULT updateHr = GAME->Update_PrimaryShadowLightDesc(lightDesc);
    if (FAILED(updateHr))
    {
        LOG_WARN("[Level_Gameplay] Update_DynamicShadowLightFromView failed. hr=0x{:08X}", static_cast<uint32>(updateHr));
    }
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
    Vec3 existingSpawnPos = Vec3::Zero;
    float existingSpawnRotY = 0.f;

    // 레벨 JSON에 저장된 PlayerStart가 이미 있으면 하드코딩 fallback을 추가로 만들지 않는다.
    if (Try_FindGameplayPlayerStartTransform(ETOI(ELevelType::GamePlay), existingSpawnPos, existingSpawnRotY))
        return S_OK;

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

void Level_Gameplay::Start_MissionClearSequence()
{
    if (_missionClearSequenceActive || _konohaTransitionRequested)
        return;

    _missionClearSequenceActive = true;
    _missionClearTransitionRequested = false;
    _missionClearTimer = 0.f;

    if (_playerHUD)
    {
        _playerHUD->Show_MissionEnd();
        _playerHUD->Set_ScreenFadeAlpha(0.f);
    }
}

void Level_Gameplay::Update_MissionClearSequence(float timeDelta)
{
    if (!_missionClearSequenceActive)
        return;

    _missionClearTimer += timeDelta;

    const float fadeStartTime = _missionClearHoldTime;
    const float fadeEndTime = _missionClearHoldTime + _missionClearFadeOutTime;

    if (_missionClearTimer >= fadeStartTime)
    {
        const float fadeRatio =
            (_missionClearTimer - fadeStartTime) / max(_missionClearFadeOutTime, FLT_EPSILON);

        if (_playerHUD)
            _playerHUD->Set_ScreenFadeAlpha(clamp(fadeRatio, 0.f, 1.f));
    }

    if (_missionClearTimer < fadeEndTime)
        return;

    Finish_MissionClearSequence();
}

void Level_Gameplay::Finish_MissionClearSequence()
{
    if (_missionClearTransitionRequested)
        return;

    _missionClearTransitionRequested = true;

    if (_playerHUD)
        _playerHUD->Set_ScreenFadeAlpha(1.f);

    Request_EnterKonoha();
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
        _surfaceProxyModels.size() +
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
    appendEntries(_surfaceProxyModels, ECollisionProxyType::Walkable);
    appendEntries(_worldBlockProxyModels, ECollisionProxyType::WorldBlock);
}

HRESULT Level_Gameplay::Rebuild_CollisionProxyCache()
{
    _surfaceProxyModels.clear();
    _worldBlockProxyModels.clear();

    CHECK_FAILED(Collect_CollisionProxyActorsFromLayer(TEXT("Layer_CollisionProxy")), E_FAIL);

    vector<FProxyEntry> proxyEntries;
    Build_CollisionProxyEntries(proxyEntries);

    GAME->Ready_CollisionProxy(proxyEntries);
    CHECK_FAILED(Register_PhysXProxiesForTest(), E_FAIL);

    const uint32 levelIndex = ETOI(ELevelType::GamePlay);
    const auto gameObjects = GAME->Get_GameObjects(levelIndex);
    for (const auto& obj : gameObjects)
    {
        if (!obj)
            continue;

        Apply_CollisionModelsToPlayer(obj);
    }

    LOG_INFO("[Level_Gameplay] Default Ground = {}", _defaultGroundModels.size());
    LOG_INFO("[Level_Gameplay] Surface Proxy = {}", _surfaceProxyModels.size());
    LOG_INFO("[Level_Gameplay] WorldBlock Proxy = {}", _worldBlockProxyModels.size());

    return S_OK;
}

HRESULT Level_Gameplay::Register_PhysXProxiesForTest()
{
    GAME->Clear_PhysXScene();

    int32 registeredCount = 0;
    int32 registeredWalkableCount = 0;
    int32 registeredWorldBlockCount = 0;

    const auto registerInstances =
        [&registeredCount, &registeredWalkableCount, &registeredWorldBlockCount, this](
            const vector<MovementComponent::FCollisionModelInstance>& instances,
            const string& debugPrefix,
            ECollisionProxyType proxyType)
        {
            uint32 instanceIndex = 0;
            for (const auto& instance : instances)
            {
                const string debugName = debugPrefix + "_" + to_string(instanceIndex);
                if (Register_PhysXProxyInstanceForTest(instance, debugName, proxyType))
                {
                    ++registeredCount;

                    if (proxyType == ECollisionProxyType::WorldBlock)
                        ++registeredWorldBlockCount;
                    else
                        ++registeredWalkableCount;
                }

                ++instanceIndex;
            }
        };

    registerInstances(_defaultGroundModels, "Gameplay_PhysX_DefaultGround", ECollisionProxyType::Walkable);
    registerInstances(_surfaceProxyModels, "Gameplay_PhysX_SurfaceProxy", ECollisionProxyType::Walkable);
    registerInstances(_worldBlockProxyModels, "Gameplay_PhysX_WorldBlockProxy", ECollisionProxyType::WorldBlock);

    LOG_INFO(
        "[Level_Gameplay] PhysX proxies registered. count={}, walkable={}, worldBlock={}",
        registeredCount,
        registeredWalkableCount,
        registeredWorldBlockCount);

    return S_OK;
}

bool Level_Gameplay::Register_PhysXProxyInstanceForTest(
    const MovementComponent::FCollisionModelInstance& instance,
    const string& debugName,
    ECollisionProxyType proxyType) const
{
    if (!instance.model)
        return false;

    vector<Vec3> vertices;
    vector<uint32> indices;

    for (const auto& mesh : instance.model->Get_Meshes())
    {
        if (!mesh)
            continue;

        const auto& meshPositions = mesh->Get_CPUPositions();
        const auto& meshIndices = mesh->Get_CPUIndices();
        if (meshPositions.empty() || meshIndices.empty())
            continue;

        const uint32 baseVertex = static_cast<uint32>(vertices.size());
        vertices.insert(vertices.end(), meshPositions.begin(), meshPositions.end());

        indices.reserve(indices.size() + meshIndices.size());
        for (const uint32 meshIndex : meshIndices)
        {
            indices.push_back(baseVertex + meshIndex);
        }
    }

    if (vertices.empty() || indices.size() < 3)
        return false;

    return GAME->Register_PhysXStaticTriangleMesh(debugName, vertices, indices, instance.worldMatrix, proxyType);
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
    case ECollisionProxyType::WallRun:
        _surfaceProxyModels.push_back(instance);
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
    Vec3 spawnPos = Vec3::Zero;
    float spawnRotY = 0.f;
    Try_FindGameplayPlayerStartTransform(ETOI(ELevelType::GamePlay), spawnPos, spawnRotY);

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
    Apply_CollisionModelsToPlayer(obj);
}

void Level_Gameplay::Apply_CollisionModelsToPlayer(const Shared<GameObject>& obj)
{
    if (!obj)
        return;

    auto moveCom = obj->Get_Component<MovementComponent>();
    if (!moveCom)
        return;

    moveCom->Set_TraceDebugEnabled(_showCollisionDebug);
}

void Level_Gameplay::Try_SendEnterGamePacket()
{
    if (_enterGameSent)
        return;

    if (!NetworkManager::GetInstance()->IsConnected())
        return;

    Vec3 spawnPos = Vec3::Zero;
    float spawnRotY = 0.f;

    // PlayerStart를 아직 못 찾은 프레임에는 (0,0,0)을 서버로 보내지 않고 다음 Update에서 다시 시도한다.
    if (!Try_FindGameplayPlayerStartTransform(ETOI(ELevelType::GamePlay), spawnPos, spawnRotY))
        return;

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

        auto moveCom = obj->Get_Component<MovementComponent>();
        if (moveCom)
            moveCom->Set_TraceDebugEnabled(_showCollisionDebug);

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
    const int32 maxSurfaceDrawCount = 80;
    const int32 maxWorldBlockDrawCount = 80;

    int32 defaultGroundDrawCount = 0;
    int32 surfaceDrawCount = 0;
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
        _surfaceProxyModels,
        Color(0.f, 1.f, 0.3f, 1.f),
        maxSurfaceDrawCount,
        surfaceDrawCount);

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

