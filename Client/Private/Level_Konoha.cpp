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
#include "WaveTrigger.h"

Level_Konoha::Level_Konoha(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

Level_Konoha::~Level_Konoha()
{

}

// Konoha도 Gameplay처럼 플레이어 주변을 좁은 ortho shadow frustum으로 추적한다.
static constexpr float KONOHA_SHADOW_TARGET_HEIGHT_OFFSET = 3.f;
// Shadow Eye는 ortho mode에서는 디버그/인스펙터 표시용으로 유지하고, 실제 범위는 Ortho Width/Height가 결정한다.
static constexpr float KONOHA_SHADOW_EYE_DISTANCE = 95.f;
// Editor에서 perspective debug로 전환했을 때 쓸 fallback FOV이며, 기본 Konoha shadow는 ortho를 사용한다.
static constexpr float KONOHA_FIXED_SHADOW_FOV_Y = 126.f;
// near가 커지면 플레이어 주변 caster가 잘릴 수 있어서 Gameplay와 같은 낮은 값을 유지한다.
static constexpr float KONOHA_FIXED_SHADOW_NEAR = 0.1f;
// far가 너무 크면 Konoha 원거리 건물/산맥이 shadow RT를 잡아먹으므로 Gameplay와 같은 값으로 제한한다.
static constexpr float KONOHA_FIXED_SHADOW_FAR = 220.f;
// Konoha도 Gameplay와 같은 texel 밀도를 쓰기 위해 shadow coverage를 동일하게 맞춘다.
static constexpr float KONOHA_SHADOW_ORTHO_WIDTH = 80.f;
// 16:9 shadow RT 기준 Gameplay에서 검증된 높이다.
static constexpr float KONOHA_SHADOW_ORTHO_HEIGHT = 45.f;

// 코노하에서 RT write가 실제로 보였던 플레이어/카메라 추적 shadow helper다.
static bool Try_BuildShadowCameraFromCurrentView(uint32 levelIndex, Vec3& outShadowEye, Vec3& outShadowTarget)
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
        outShadowTarget = playerPosition + Vec3(0.f, KONOHA_SHADOW_TARGET_HEIGHT_OFFSET, 0.f);
        hasTarget = true;
        break;
    }

    if (hasTarget)
    {
        outShadowEye = outShadowTarget + Vec3(0.f, 18.f, -14.f);
        return true;
    }

    // 플레이어 생성 전 첫 프레임에도 shadow matrix가 산맥/월드 원거리로 튀지 않도록 Konoha 기본 원점 근처를 본다.
    outShadowTarget = Vec3(0.f, KONOHA_SHADOW_TARGET_HEIGHT_OFFSET, 0.f);
    outShadowEye = outShadowTarget + Vec3(0.f, 18.f, -14.f);
    return true;
}

// Konoha 레벨에서 서버 입장/로컬 스폰/PhysX 테스트 기준점을 같은 PlayerStart로 맞출 때 호출한다.
// 레벨 JSON에 저장된 PlayerStart를 우선 사용하고, 아직 준비되지 않았으면 false를 반환한다.
static bool Try_FindKonohaPlayerStartTransform(uint32 levelIndex, Vec3& outSpawnPos, float& outSpawnRotY)
{
    outSpawnPos = Vec3::Zero;
    outSpawnRotY = 0.f;
    // PlayerStart가 중복 저장된 경우 파일 뒤쪽에 있는 최신 항목을 최종 스폰 기준으로 사용한다.
    bool foundPlayerStart = false;

    const auto gameObjects = GAME->Get_GameObjects(levelIndex);
    for (const auto& obj : gameObjects)
    {
        if (!obj || obj->Get_ObjectType() != Protocol::OBJECT_TYPE_PLAYER_START)
            continue;

        auto transform = obj->Get_Transform();
        if (!transform)
            continue;

        outSpawnPos = transform->Get_WorldPosition();
        // Quaternion::ToEuler는 radian을 반환하므로 Transform/패킷에서 쓰는 degree yaw로 변환한다.
        const Vec3 playerStartEulerRad = transform->Get_WorldRotation().ToEuler();
        outSpawnRotY = XMConvertToDegrees(playerStartEulerRad.y);
        foundPlayerStart = true;
    }

    return foundPlayerStart;
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

    return S_OK;
}

void Level_Konoha::Update(float timeDelta)
{
    Level::Update(timeDelta);
    Update_DynamicShadowLightFromView();

    Refresh_NearbyCollisionModels(timeDelta);

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

HRESULT Level_Konoha::On_LevelChunkLoaded(const wstring& fileName)
{
    CHECK_FAILED(Rebuild_CollisionProxyCache(), E_FAIL);
    CHECK_FAILED(Register_PhysXProxiesForTest(Vec3(0.f, 20.3f, -12.2f)), E_FAIL);

    // Konoha 청크가 늦게 붙으면 static mesh caster가 바뀌므로 static shadow RT를 다시 찍어야 한다.
    GAME->Invalidate_StaticShadowMap();

    return S_OK;
}

Matrix Level_Konoha::Build_CollisionModelPreTransform()
{
    Matrix scaleMatrix = Matrix::CreateScale(1.f);
    Matrix rotationMatrix = Matrix::CreateRotationY(XMConvertToRadians(180.f));

    return scaleMatrix * rotationMatrix;
}

// Ground_Collision 폴더 안에서도 바닥 계열 meshbin만 기본 걷기 충돌로 사용한다.
// 땅이 아닌 충돌 메시가 섞여 들어와도 이름에 ground가 없는 파일은 제외된다.
static bool IsGroundCollisionMeshFile(const fs::path& meshPath)
{
    const string extensionLower = Utils::ToLowerCopy(meshPath.extension().string());
    if (extensionLower != ".meshbin")
        return false;

    const string fileNameLower = Utils::ToLowerCopy(meshPath.filename().string());
    return (fileNameLower.find("ground") != string::npos);
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
    // Konoha 기본 shadow 설정이 Editor Override에 막히지 않도록 레벨 진입 시 override를 초기화한다.
    GAME->Set_EditorPrimaryShadowLightOverrideEnabled(false);

    // 새 레벨 라이트를 등록하기 전에 이전 레벨에서 남은 라이트를 정리한다.
    GAME->Clear_Lights();

    FLightDesc lightDesc{};

    lightDesc.type = ELightType::Directional;
    lightDesc.direction = Vec4(-1.f, -1.f, -1.f, 0.f);
    lightDesc.diffuse = Vec4(0.92f, 0.74f, 0.58f, 1.f);
    lightDesc.ambient = Vec4(0.14f, 0.12f, 0.16f, 1.f);
    lightDesc.specular = Vec4(0.42f, 0.32f, 0.25f, 1.f);

    lightDesc.castShadow = true;
    lightDesc.shadowMapSize = 2048;
    lightDesc.shadowCenter = Vec3::Zero;
    lightDesc.shadowOrthoWidth = KONOHA_SHADOW_ORTHO_WIDTH;
    lightDesc.shadowOrthoHeight = KONOHA_SHADOW_ORTHO_HEIGHT;
    lightDesc.useShadowCamera = false;
    lightDesc.shadowTarget = Vec3::Zero;
    lightDesc.shadowFovY = XMConvertToRadians(KONOHA_FIXED_SHADOW_FOV_Y);
    lightDesc.shadowAspect = max(1.f, GAME->Get_ViewportWidth()) / max(1.f, GAME->Get_ViewportHeight());
    lightDesc.shadowNear = KONOHA_FIXED_SHADOW_NEAR;
    lightDesc.shadowFar = KONOHA_FIXED_SHADOW_FAR;
    lightDesc.shadowBias = 0.00025f;
    lightDesc.shadowStrength = 0.68f;
    lightDesc.shadowSoftness = 0.35f;

    Vec3 lightDir = Vec3(lightDesc.direction.x, lightDesc.direction.y, lightDesc.direction.z); // 수업코드처럼 shadow camera를 directional light 방향 축으로 배치하기 위한 광원 방향 벡터다.
    if (lightDir.LengthSquared() <= FLT_EPSILON)
        lightDir = Vec3(-1.f, -1.f, -1.f);
    else
        lightDir.Normalize();

    lightDesc.shadowEye = lightDesc.shadowTarget - lightDir * KONOHA_SHADOW_EYE_DISTANCE;

    CHECK_FAILED(GAME->Add_Light(lightDesc), E_FAIL);

    return S_OK;
}

void Level_Konoha::Update_DynamicShadowLightFromView()
{
    FLightDesc lightDesc{};

    lightDesc.type = ELightType::Directional;
    lightDesc.direction = Vec4(-1.f, -1.f, -1.f, 0.f);
    lightDesc.diffuse = Vec4(0.92f, 0.74f, 0.58f, 1.f);
    lightDesc.ambient = Vec4(0.14f, 0.12f, 0.16f, 1.f);
    lightDesc.specular = Vec4(0.42f, 0.32f, 0.25f, 1.f);

    lightDesc.castShadow = true;
    lightDesc.shadowMapSize = 2048;
    lightDesc.shadowCenter = Vec3::Zero;
    lightDesc.shadowOrthoWidth = KONOHA_SHADOW_ORTHO_WIDTH;
    lightDesc.shadowOrthoHeight = KONOHA_SHADOW_ORTHO_HEIGHT;
    lightDesc.useShadowCamera = false;
    lightDesc.shadowTarget = Vec3::Zero;
    lightDesc.shadowFovY = XMConvertToRadians(KONOHA_FIXED_SHADOW_FOV_Y);
    lightDesc.shadowAspect = max(1.f, GAME->Get_ViewportWidth()) / max(1.f, GAME->Get_ViewportHeight());
    lightDesc.shadowNear = KONOHA_FIXED_SHADOW_NEAR;
    lightDesc.shadowFar = KONOHA_FIXED_SHADOW_FAR;
    lightDesc.shadowBias = 0.00025f;
    lightDesc.shadowStrength = 0.68f;
    lightDesc.shadowSoftness = 0.35f;

    Vec3 dynamicShadowEye = Vec3::Zero;
    Vec3 dynamicShadowTarget = Vec3::Zero;
    if (Try_BuildShadowCameraFromCurrentView(ETOI(ELevelType::Konoha), dynamicShadowEye, dynamicShadowTarget))
    {
        lightDesc.shadowTarget = dynamicShadowTarget;
        lightDesc.shadowCenter = dynamicShadowTarget;
    }

    Vec3 lightDir = Vec3(lightDesc.direction.x, lightDesc.direction.y, lightDesc.direction.z); // active camera 위치 대신 directional light 방향으로 shadow eye를 다시 맞추기 위한 벡터다.
    if (lightDir.LengthSquared() <= FLT_EPSILON)
        lightDir = Vec3(-1.f, -1.f, -1.f);
    else
        lightDir.Normalize();

    // 수업코드처럼 shadow target만 추적하고, shadow eye는 항상 빛 방향 축 위에 둔다.
    lightDesc.shadowEye = lightDesc.shadowTarget - lightDir * KONOHA_SHADOW_EYE_DISTANCE;

    const HRESULT updateHr = GAME->Update_PrimaryShadowLightDesc(lightDesc);
    if (FAILED(updateHr))
    {
        LOG_WARN("[Level_Konoha] Update_DynamicShadowLightFromView failed. hr=0x{:08X}", static_cast<uint32>(updateHr));
    }
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
    Vec3 existingSpawnPos = Vec3::Zero;
    float existingSpawnRotY = 0.f;

    // 레벨에 저장된 PlayerStart가 이미 있으면 그 좌표를 그대로 쓰고 fallback 생성은 건너뛴다.
    if (Try_FindKonohaPlayerStartTransform(levelIndex, existingSpawnPos, existingSpawnRotY))
    {
        CHECK_FAILED(Register_PhysXProxiesForTest(existingSpawnPos), E_FAIL);
        return S_OK;
    }

    // TODO : Spawn Point Save&Load로 위치 세팅
    {
        PlayerStart::FPlayerStartDesc desc;
        desc.position = Vec3(0.f, 20.3f, -12.2f);
        desc.spawnIndex = 0;

        CHECK_FAILED(GAME->Add_GameObject(levelIndex, Protocol::OBJECT_TYPE_PLAYER_START, layerTag, &desc), E_FAIL);
        CHECK_FAILED(Register_PhysXProxiesForTest(desc.position), E_FAIL);
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
        desc.colorTint = Vec4(0.95f, 0.9f, 0.86f, 1.f);
        desc.horizonColor = Vec4(0.92f, 0.54f, 0.33f, 1.f);
        desc.zenithColor = Vec4(0.14f, 0.18f, 0.32f, 1.f);
        desc.opacity = 1.f;
        desc.emissiveStrength = 0.82f;
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
        desc.colorTint = Vec4(1.f, 0.82f, 0.72f, 0.92f);
        desc.subUVTiling = Vec2(1.f, 1.f);
        desc.subUVScrollSpeed = Vec2::Zero;
        desc.opacity = 0.24f;
        desc.emissiveStrength = 0.72f;
        desc.scale = Vec3(1.f, 1.f, 1.f);

        CHECK_FAILED(GAME->Add_GameObject(levelIndex, Protocol::OBJECT_TYPE_SKY_SPHERE, layerTag, &desc), E_FAIL);
    }

    // 구름은 한개만 있어도 적당히 자연스러운듯
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
    // 플레이어 허드 생성 -> 레벨 전환 시 문제. 기존꺼 제대로 안비워지는거같은데 왜지
    {
        UIObject::FUIDesc desc;
        desc.posX = 0.f;
        desc.posY = 0.f;
        desc.sizeX = 1.f;
        desc.sizeY = 1.f;
        desc.zOrder = 0.5f;
        desc.levelIndex = ETOI(ELevelType::Konoha);

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

void Level_Konoha::Query_CollisionProxyBoundsModels(
    const BoundingBox& queryBounds,
    vector<MovementComponent::FCollisionModelInstance>& outSurfaceModels,
    vector<MovementComponent::FCollisionModelInstance>& outWorldBlockModels) const
{
    if (_collisionProxyCellSize <= 0.f || _collisionProxyCellModels.empty())
        return;

    const Vec3 boundsCenter = Vec3(
        queryBounds.Center.x,
        queryBounds.Center.y,
        queryBounds.Center.z);
    const Vec3 boundsExtents = Vec3(
        queryBounds.Extents.x,
        queryBounds.Extents.y,
        queryBounds.Extents.z);
    const Vec3 minPos = boundsCenter - boundsExtents;
    const Vec3 maxPos = boundsCenter + boundsExtents;

    const int32 minCellX = static_cast<int32>(floorf(minPos.x / _collisionProxyCellSize));
    const int32 maxCellX = static_cast<int32>(floorf(maxPos.x / _collisionProxyCellSize));
    const int32 minCellZ = static_cast<int32>(floorf(minPos.z / _collisionProxyCellSize));
    const int32 maxCellZ = static_cast<int32>(floorf(maxPos.z / _collisionProxyCellSize));

    uset<uint32> addedSurfaceIndices;
    uset<uint32> addedWorldBlockIndices;

    for (int32 cellZ = minCellZ; cellZ <= maxCellZ; ++cellZ)
    {
        for (int32 cellX = minCellX; cellX <= maxCellX; ++cellX)
        {
            FCollisionCellCoord cell{};
            cell.x = cellX;
            cell.z = cellZ;

            const auto iter = _collisionProxyCellModels.find(cell);
            if (iter == _collisionProxyCellModels.end())
                continue;

            for (uint32 surfaceIndex : iter->second.surfaceIndices)
            {
                if (!addedSurfaceIndices.insert(surfaceIndex).second)
                    continue;

                if (surfaceIndex >= _surfaceProxyModels.size())
                    continue;

                const auto& instance = _surfaceProxyModels[surfaceIndex];
                if (instance.hasWorldBounds && !queryBounds.Intersects(instance.worldBounds))
                    continue;

                outSurfaceModels.push_back(instance);
            }

            for (uint32 worldBlockIndex : iter->second.worldBlockIndices)
            {
                if (!addedWorldBlockIndices.insert(worldBlockIndex).second)
                    continue;

                if (worldBlockIndex >= _worldBlockProxyModels.size())
                    continue;

                const auto& instance = _worldBlockProxyModels[worldBlockIndex];
                if (instance.hasWorldBounds && !queryBounds.Intersects(instance.worldBounds))
                    continue;

                outWorldBlockModels.push_back(instance);
            }
        }
    }
}

void Level_Konoha::Disable_LocalWaveTriggers_ForServerMode()
{
    const uint32 levelIndex = ETOI(ELevelType::Konoha);
    const auto gameObjects = GAME->Get_GameObjects(levelIndex);

    for (const auto& obj : gameObjects)
    {
        auto waveTrigger = dynamic_pointer_cast<WaveTrigger>(obj);
        if (!waveTrigger)
            continue;

        waveTrigger->Set_ServerAuthoritative(true);
    }
}

void Level_Konoha::Build_CollisionProxyEntries(vector<FProxyEntry>& outEntries) const
{
    outEntries.clear();
    outEntries.reserve(
        _defaultGroundModels.size() +
        _surfaceProxyModels.size() +
        _worldBlockProxyModels.size());

    const auto appendEntries =
        [&outEntries](
            const vector<MovementComponent::FCollisionModelInstance>& instances,
            ECollisionProxyType proxyType)
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


HRESULT Level_Konoha::Ready_DefaultGroundCollision()
{
    _defaultGroundModels.clear();

    const fs::path groundDirectory =
        L"../../Client/Bin/Resources/StaticMesh/KonohaVillage/Meshes/Ground_Collision";

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
        if (!IsGroundCollisionMeshFile(meshPath))
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
    _surfaceProxyModels.clear();
    _surfaceProxyTypes.clear();
    _worldBlockProxyModels.clear();
    _collisionProxyCellModels.clear();

    CHECK_FAILED(Collect_CollisionProxyActorsFromLayer(TEXT("Layer_CollisionProxy")), E_FAIL);
    CHECK_FAILED(Build_CollisionProxyCellModels(), E_FAIL);

    vector<FProxyEntry> proxyEntries;
    Build_CollisionProxyEntries(proxyEntries);

    GAME->Ready_CollisionProxy(proxyEntries);
    Refresh_PlayerCollisionModels();

    LOG_INFO("[Level_Konoha] Surface Proxy = {}", _surfaceProxyModels.size());
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

HRESULT Level_Konoha::Build_CollisionProxyCellModels()
{
    _collisionProxyCellModels.clear();

    if (_collisionProxyCellSize <= 0.f)
        return E_FAIL;

    const auto appendInstances =
        [&](const vector<MovementComponent::FCollisionModelInstance>& instances, bool isSurface)
        {
            for (uint32 instanceIndex = 0; instanceIndex < instances.size(); ++instanceIndex)
            {
                const auto& instance = instances[instanceIndex];
                if (!instance.hasWorldBounds)
                    continue;

                const Vec3 boundsCenter = Vec3(
                    instance.worldBounds.Center.x,
                    instance.worldBounds.Center.y,
                    instance.worldBounds.Center.z);
                const Vec3 boundsExtents = Vec3(
                    instance.worldBounds.Extents.x,
                    instance.worldBounds.Extents.y,
                    instance.worldBounds.Extents.z);
                const Vec3 minPos = boundsCenter - boundsExtents;
                const Vec3 maxPos = boundsCenter + boundsExtents;

                const int32 minCellX = static_cast<int32>(floorf(minPos.x / _collisionProxyCellSize));
                const int32 maxCellX = static_cast<int32>(floorf(maxPos.x / _collisionProxyCellSize));
                const int32 minCellZ = static_cast<int32>(floorf(minPos.z / _collisionProxyCellSize));
                const int32 maxCellZ = static_cast<int32>(floorf(maxPos.z / _collisionProxyCellSize));

                for (int32 cellZ = minCellZ; cellZ <= maxCellZ; ++cellZ)
                {
                    for (int32 cellX = minCellX; cellX <= maxCellX; ++cellX)
                    {
                        FCollisionCellCoord cell{};
                        cell.x = cellX;
                        cell.z = cellZ;

                        auto& bucket = _collisionProxyCellModels[cell];
                        if (isSurface)
                            bucket.surfaceIndices.push_back(instanceIndex);
                        else
                            bucket.worldBlockIndices.push_back(instanceIndex);
                    }
                }
            }
        };

    appendInstances(_surfaceProxyModels, true);
    appendInstances(_worldBlockProxyModels, false);

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
    case ECollisionProxyType::WallRun:
        _surfaceProxyModels.push_back(instance);
        _surfaceProxyTypes.push_back(actor->Get_ProxyType());
        break;

    case ECollisionProxyType::WorldBlock:
        _worldBlockProxyModels.push_back(instance);
        break;

    default:
        break;
    }

    return S_OK;
}

HRESULT Level_Konoha::Register_PhysXProxiesForTest(const Vec3& playerStartPos)
{
    // PhysX 검증 단계에서는 가까운 거리 제한 없이 proxy 전체를 등록해서 ray miss 원인을 분리한다.
    GAME->Clear_PhysXScene();
    _hasPhysXTestProxy = false;
    _physXTestProxyCenter = Vec3::Zero;

    const MovementComponent::FCollisionModelInstance* nearestInstance = nullptr;
    float nearestDistanceSq = FLT_MAX;
    int32 registeredCount = 0;
    int32 registeredWalkableCount = 0;
    int32 registeredWallRunCount = 0;
    int32 registeredWorldBlockCount = 0;

    const auto registerInstances =
        [&playerStartPos, &nearestInstance, &nearestDistanceSq,
            &registeredCount, &registeredWalkableCount, &registeredWallRunCount, &registeredWorldBlockCount, this](
            const vector<MovementComponent::FCollisionModelInstance>& instances,
            const string& debugPrefix,
            ECollisionProxyType fallbackProxyType,
            const vector<ECollisionProxyType>* proxyTypes = nullptr)
        {
            uint32 instanceIndex = 0;
            for (const auto& instance : instances)
            {
                if (!instance.model || !instance.hasWorldBounds)
                {
                    ++instanceIndex;
                    continue;
                }

                const Vec3 toCenter = instance.worldBounds.Center - playerStartPos;
                const float distanceSq = toCenter.LengthSquared();
                const string debugName = debugPrefix + "_" + to_string(instanceIndex);
                ECollisionProxyType proxyType = fallbackProxyType;
                if (proxyTypes && instanceIndex < proxyTypes->size())
                    proxyType = (*proxyTypes)[instanceIndex];

                if (Register_PhysXProxyInstanceForTest(instance, debugName, proxyType))
                {
                    ++registeredCount;

                    switch (proxyType)
                    {
                    case ECollisionProxyType::Walkable:
                        ++registeredWalkableCount;
                        break;
                    case ECollisionProxyType::WallRun:
                        ++registeredWallRunCount;
                        break;
                    case ECollisionProxyType::WorldBlock:
                        ++registeredWorldBlockCount;
                        break;
                    default:
                        break;
                    }

                    if (distanceSq < nearestDistanceSq)
                    {
                        nearestDistanceSq = distanceSq;
                        nearestInstance = &instance;
                    }
                }

                ++instanceIndex;
            }
        };

    registerInstances(_defaultGroundModels, "Konoha_PhysX_DefaultGround", ECollisionProxyType::Walkable);
    registerInstances(_surfaceProxyModels, "Konoha_PhysX_SurfaceProxy", ECollisionProxyType::WallRun, &_surfaceProxyTypes);
    registerInstances(_worldBlockProxyModels, "Konoha_PhysX_WorldBlockProxy", ECollisionProxyType::WorldBlock);

    if (registeredCount <= 0 || !nearestInstance)
    {
        LOG_WARN("[Level_Konoha] PhysX test proxy registration found no valid proxy.");
        return S_OK;
    }

    LOG_INFO(
        "[Level_Konoha] PhysX test proxies registered. count={}, walkable={}, wallRun={}, worldBlock={}, nearestCenter=({}, {}, {}), nearestDistance={}",
        registeredCount,
        registeredWalkableCount,
        registeredWallRunCount,
        registeredWorldBlockCount,
        nearestInstance->worldBounds.Center.x,
        nearestInstance->worldBounds.Center.y,
        nearestInstance->worldBounds.Center.z,
        sqrtf(nearestDistanceSq));

    _physXTestProxyCenter = nearestInstance->worldBounds.Center;
    _hasPhysXTestProxy = true;

    return S_OK;
}

bool Level_Konoha::Register_PhysXProxyInstanceForTest(
    const MovementComponent::FCollisionModelInstance& instance,
    const string& debugName,
    ECollisionProxyType proxyType) const
{
    // Model 안의 여러 mesh를 하나의 vertex/index 배열로 합쳐 PhysX triangle mesh 하나로 등록한다.
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
    Vec3 debugForward = Vec3(0.f, 0.f, 1.f);
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
        debugForward = transform->Get_WorldForward();

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
            debugForward = activeCamera->Get_Transform()->Get_WorldForward();
            hasDebugCenter = true;
        }
    }

    if (!hasDebugCenter)
        return;

    const float debugRadius = 120.f;
    const float debugRadiusSq = debugRadius * debugRadius;

    const int32 maxDefaultGroundDrawCount = 120;
    const int32 maxSurfaceDrawCount = 120;
    const int32 maxWorldBlockDrawCount = 80;

    int32 defaultGroundDrawCount = 0;
    int32 surfaceDrawCount = 0;
    int32 worldBlockDrawCount = 0;

    const auto drawProxyBoxes =
        [debugCenter, debugRadiusSq](
            const vector<MovementComponent::FCollisionModelInstance>& instances,
            const Color& color,
            bool depthEnabled,
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
                debugBoxDesc.style.depthEnabled = depthEnabled;

                GAME->Draw_DebugBox(debugBoxDesc);
                ++drawCount;
            }
        };

    drawProxyBoxes(
        _defaultGroundModels,
        Color(1.f, 0.f, 0.2f, 1.f),
        true,
        maxDefaultGroundDrawCount,
        defaultGroundDrawCount);

    drawProxyBoxes(
        _surfaceProxyModels,
        Color(0.f, 1.f, 0.3f, 1.f),
        true,
        maxSurfaceDrawCount,
        surfaceDrawCount);

    drawProxyBoxes(
        _worldBlockProxyModels,
        Color(0.f, 0.6f, 1.f, 1.f),
        true,
        maxWorldBlockDrawCount,
        worldBlockDrawCount);

    FDebugSphereDesc debugSphereDesc{};
    debugSphereDesc.center = debugCenter;
    debugSphereDesc.radius = 0.18f;
    debugSphereDesc.style.color = Color(1.f, 1.f, 1.f, 1.f);
    debugSphereDesc.style.duration = 0.f;
    debugSphereDesc.style.depthEnabled = false;

    GAME->Draw_DebugSphere(debugSphereDesc);

    if (debugForward.LengthSquared() > 0.000001f)
        debugForward.Normalize();

    const Vec3 rayStart = debugCenter + Vec3(0.f, 1.0f, 0.f);
    const float rayDistance = 30.f;

    FPhysXRaycastHit physXHit{};
    const bool physXHitResult = GAME->Raycast_PhysX(rayStart, debugForward, rayDistance, physXHit);

    FDebugLineDesc physXRayDesc{};
    physXRayDesc.start = rayStart;
    physXRayDesc.end = physXHitResult ? physXHit.position : rayStart + debugForward * rayDistance;
    physXRayDesc.style.color = physXHitResult
        ? Color(1.f, 0.9f, 0.f, 1.f)
        : Color(1.f, 0.1f, 0.1f, 1.f);
    physXRayDesc.style.duration = 0.f;
    physXRayDesc.style.depthEnabled = false;
    GAME->Draw_DebugLine(physXRayDesc);

    if (physXHitResult)
    {
        FDebugSphereDesc hitSphereDesc{};
        hitSphereDesc.center = physXHit.position;
        hitSphereDesc.radius = 0.28f;
        hitSphereDesc.style.color = Color(1.f, 0.9f, 0.f, 1.f);
        hitSphereDesc.style.duration = 0.f;
        hitSphereDesc.style.depthEnabled = false;
        GAME->Draw_DebugSphere(hitSphereDesc);

        FDebugLineDesc normalDesc{};
        normalDesc.start = physXHit.position;
        normalDesc.end = physXHit.position + physXHit.normal * 1.5f;
        normalDesc.style.color = Color(0.f, 0.8f, 1.f, 1.f);
        normalDesc.style.duration = 0.f;
        normalDesc.style.depthEnabled = false;
        GAME->Draw_DebugLine(normalDesc);
    }

    if (_hasPhysXTestProxy)
    {
        const Vec3 toTestProxy = _physXTestProxyCenter - rayStart;
        const float testProxyDistance = toTestProxy.Length();

        if (testProxyDistance > 0.001f)
        {
            Vec3 testProxyDir = toTestProxy / testProxyDistance;
            FPhysXRaycastHit testProxyHit{};
            const bool testProxyHitResult = GAME->Raycast_PhysX(
                rayStart,
                testProxyDir,
                testProxyDistance + 3.f,
                testProxyHit);

            FDebugLineDesc testProxyRayDesc{};
            testProxyRayDesc.start = rayStart;
            testProxyRayDesc.end = testProxyHitResult ? testProxyHit.position : _physXTestProxyCenter;
            testProxyRayDesc.style.color = testProxyHitResult
                ? Color(0.2f, 1.f, 1.f, 1.f)
                : Color(1.f, 0.f, 1.f, 1.f);
            testProxyRayDesc.style.duration = 0.f;
            testProxyRayDesc.style.depthEnabled = false;
            GAME->Draw_DebugLine(testProxyRayDesc);

            FDebugSphereDesc testProxyCenterDesc{};
            testProxyCenterDesc.center = _physXTestProxyCenter;
            testProxyCenterDesc.radius = 0.25f;
            testProxyCenterDesc.style.color = Color(0.2f, 1.f, 1.f, 1.f);
            testProxyCenterDesc.style.duration = 0.f;
            testProxyCenterDesc.style.depthEnabled = false;
            GAME->Draw_DebugSphere(testProxyCenterDesc);
        }
    }
}

void Level_Konoha::Spawn_LocalPlayer()
{
    const uint32 levelIndex = ETOI(ELevelType::Konoha);
    // 레벨 JSON에 저장된 PlayerStart 위치를 로컬 플레이어의 최초 스폰 기준으로 사용한다.
    Vec3 spawnPos = Vec3::Zero;
    // PlayerStart yaw를 함께 적용해 로컬/서버 진입 시 바라보는 방향 차이를 없앤다.
    float spawnRotY = 0.f;
    Try_FindKonohaPlayerStartTransform(levelIndex, spawnPos, spawnRotY);

    auto playerObj = Spawn_Helper::Prefab("TestPlayer2")
        .AtLevel(levelIndex)
        .Position(spawnPos)
        .Rotation(Vec3(0.f, spawnRotY, 0.f))
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
    Apply_CollisionModelsToPlayer(obj);
}

void Level_Konoha::Refresh_PlayerCollisionModels()
{
    const uint32 levelIndex = ETOI(ELevelType::Konoha);
    const auto gameObjects = GAME->Get_GameObjects(levelIndex);

    for (const auto& obj : gameObjects)
    {
        Apply_CollisionModelsToPlayer(obj);
    }
}

void Level_Konoha::Refresh_NearbyCollisionModels(float timeDelta)
{
    _collisionModelRefreshAccumulator += timeDelta;

    // 주변 프록시 재조회는 너무 자주 할 필요가 없어서 0.1초 단위로만 갱신한다.
    if (_collisionModelRefreshAccumulator < 0.1f)
        return;

    _collisionModelRefreshAccumulator = 0.f;
    Refresh_PlayerCollisionModels();
}

void Level_Konoha::Apply_CollisionModelsToPlayer(const Shared<GameObject>& obj)
{
    if (!obj)
        return;

    auto moveCom = obj->Get_Component<MovementComponent>();
    if (!moveCom)
        return;

    moveCom->Set_TraceDebugEnabled(_showCollisionDebug);
}

void Level_Konoha::Try_SendEnterGamePacket()
{
    if (_enterGameSent)
        return;

    if (!NetworkManager::GetInstance()->IsConnected())
        return;

    const uint32 levelIndex = ETOI(ELevelType::Konoha);

    Vec3 spawnPos = Vec3::Zero;
    float spawnRotY = 0.f;

    // PlayerStart를 아직 못 찾았으면 0,0,0 fallback 패킷을 보내지 않고 다음 프레임에 다시 시도한다.
    if (!Try_FindKonohaPlayerStartTransform(levelIndex, spawnPos, spawnRotY))
        return;

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

        if (obj->Get_ObjectType() != Protocol::OBJECT_TYPE_MONSTER &&
            obj->Get_ObjectType() != Protocol::OBJECT_TYPE_BOSS_PAIN)
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

