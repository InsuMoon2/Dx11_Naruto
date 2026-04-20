#include "pch.h"
#include "Skill_ShinsuSenju.h"
#include "GameObject_Factory.h"
#include "GameObject.h"
#include "Collider.h"
#include "SkillObject_Projectile.h"
#include "MovementComponent.h"
#include "Shader.h"
#include "Model.h"
#include "Debug_Manager.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_ShinsuSenju, Protocol::OBJECT_TYPE_SHINSUSENJU, "SkillSpawn");

// 진수천수 팔의 기준 포즈에서 쓰는 로컬 축을 임의 방향으로 안정적으로 재정렬할 때 호출한다.
static Vec3 Rotate_ShinsuSenjuArmVector(const Vec3& vector, const Quat& rotation)
{
    return Vec3::TransformNormal(vector, Matrix::CreateFromQuaternion(rotation));
}

// 목표 축으로 보내기 전 벡터를 평면 위에 안정적으로 투영하고, 평행한 경우 fallback 축을 사용하기 위해 호출한다.
static Vec3 Project_ShinsuSenjuArmVectorOnPlane(const Vec3& vector, const Vec3& planeNormal, const Vec3& fallback)
{
    Vec3 projected = vector - planeNormal * vector.Dot(planeNormal);
    if (projected.LengthSquared() <= FLT_EPSILON)
        return Utils::Safe_Normalize(fallback, Vec3::Up);

    projected.Normalize();
    return projected;
}

// 두 방향 벡터를 가장 짧은 경로로 정렬하는 회전을 만들기 위해 호출한다.
static Quat Create_ShinsuSenjuArmRotationBetweenVectors(const Vec3& fromVector, const Vec3& toVector)
{
    const Vec3 from = Utils::Safe_Normalize(fromVector, Vec3(0.f, 0.f, -1.f));
    const Vec3 to = Utils::Safe_Normalize(toVector, Vec3(0.f, 0.f, -1.f));

    const float dot = max(-1.f, min(1.f, from.Dot(to)));
    if (dot >= 0.9999f)
        return Quat::Identity;

    if (dot <= -0.9999f)
    {
        Vec3 axis = from.Cross(Vec3::Up);
        if (axis.LengthSquared() <= FLT_EPSILON)
            axis = from.Cross(Vec3::Right);

        axis.Normalize();
        return Quat::CreateFromAxisAngle(axis, XM_PI);
    }

    Vec3 axis = from.Cross(to);
    if (axis.LengthSquared() <= FLT_EPSILON)
        return Quat::Identity;

    axis.Normalize();
    return Quat::CreateFromAxisAngle(axis, acosf(dot));
}

// forward 를 맞춘 뒤 남는 roll 자유도를 축 기준 signed angle 로 고정할 때 호출한다.
static float Compute_ShinsuSenjuArmSignedAngleAroundAxis(const Vec3& fromVector, const Vec3& toVector, const Vec3& axis)
{
    const Vec3 safeFrom = Utils::Safe_Normalize(fromVector, Vec3::Up);
    const Vec3 safeTo = Utils::Safe_Normalize(toVector, Vec3::Up);
    const Vec3 safeAxis = Utils::Safe_Normalize(axis, Vec3::Forward);

    const float sinValue = safeAxis.Dot(safeFrom.Cross(safeTo));
    const float cosValue = max(-1.f, min(1.f, safeFrom.Dot(safeTo)));
    return atan2f(sinValue, cosValue);
}

// 회전 곱셈 순서가 다른 두 후보 중 실제 target forward/up 정렬이 더 잘 되는 쪽을 선택할 때 호출한다.
static float Evaluate_ShinsuSenjuArmRotationScore(const Quat& rotation,
    const Vec3& referenceForward,
    const Vec3& referenceUp,
    const Vec3& targetForward,
    const Vec3& targetUp)
{
    const Vec3 rotatedForward = Utils::Safe_Normalize(Rotate_ShinsuSenjuArmVector(referenceForward, rotation), targetForward);
    const Vec3 rotatedUp = Utils::Safe_Normalize(Rotate_ShinsuSenjuArmVector(referenceUp, rotation), targetUp);

    const Vec3 projectedRotatedUp = Project_ShinsuSenjuArmVectorOnPlane(rotatedUp, targetForward, targetUp);
    const float forwardScore = rotatedForward.Dot(targetForward);
    const float upScore = projectedRotatedUp.Dot(targetUp);

    return (forwardScore * 2.f) + upScore;
}

// 에디터에서 맞춘 기준 포즈 0,98,-47을 launchDirection 방향으로 재정렬한 최종 월드 회전을 만들 때 호출한다.
static Quat Build_ShinsuSenjuArmSpawnRotation(const Vec3& launchDirection)
{
    const Vec3 targetForward = Utils::Safe_Normalize(-launchDirection, Vec3(0.f, 1.f, 0.f));
    const Vec3 targetUp = Project_ShinsuSenjuArmVectorOnPlane(Vec3::Up, targetForward, Vec3::Forward);

    const Quat referencePoseRotation = Quat::CreateFromYawPitchRoll(
        XMConvertToRadians(98.f),
        XMConvertToRadians(0.f),
        XMConvertToRadians(-47.f));

    const Vec3 referenceForward = Utils::Safe_Normalize(
        Rotate_ShinsuSenjuArmVector(Vec3(0.f, 0.f, -1.f), referencePoseRotation),
        Vec3(0.f, 0.f, -1.f));
    const Vec3 referenceUp = Utils::Safe_Normalize(
        Rotate_ShinsuSenjuArmVector(Vec3::Up, referencePoseRotation),
        Vec3::Up);

    const Quat forwardAlignment = Create_ShinsuSenjuArmRotationBetweenVectors(referenceForward, targetForward);

    const Vec3 alignedUp = Utils::Safe_Normalize(
        Rotate_ShinsuSenjuArmVector(referenceUp, forwardAlignment),
        targetUp);
    const Vec3 alignedPlaneUp = Project_ShinsuSenjuArmVectorOnPlane(alignedUp, targetForward, targetUp);

    const float twistAngle = Compute_ShinsuSenjuArmSignedAngleAroundAxis(alignedPlaneUp, targetUp, targetForward);
    const Quat twistAlignment = Quat::CreateFromAxisAngle(targetForward, twistAngle);

    Quat candidateA = twistAlignment * forwardAlignment;
    candidateA.Normalize();

    Quat candidateB = forwardAlignment * twistAlignment;
    candidateB.Normalize();

    const float scoreA = Evaluate_ShinsuSenjuArmRotationScore(candidateA, referenceForward, referenceUp, targetForward, targetUp);
    const float scoreB = Evaluate_ShinsuSenjuArmRotationScore(candidateB, referenceForward, referenceUp, targetForward, targetUp);

    return (scoreA >= scoreB) ? candidateA : candidateB;
}

Skill_ShinsuSenju::Skill_ShinsuSenju(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject(device, context)
{
}

Skill_ShinsuSenju::Skill_ShinsuSenju(const Skill_ShinsuSenju& rhs)
    : SkillObject(rhs)
    , _shader(rhs._shader)
    , _model(rhs._model)
    , _initialFireDelay(rhs._initialFireDelay)
    , _burstInterval(rhs._burstInterval)
    , _maxBurstCount(rhs._maxBurstCount)
    , _forwardRange(rhs._forwardRange)
    , _impactRadius(rhs._impactRadius)
    , _armSpeed(rhs._armSpeed)
    , _burstCenter(rhs._burstCenter)
    , _delayElapsed(0.f)
    , _burstCount(0)
    , _isBurstFinished(false)
{

}

HRESULT Skill_ShinsuSenju::Initialize_Prototype()
{
    _lifetime = 10.f;

    _colliderRadius = 0.f;
    _collisionPreset = Collision_Preset::Enviroment;

    return SkillObject::Initialize_Prototype();
}

HRESULT Skill_ShinsuSenju::Initialize(void* arg)
{
    CHECK_FAILED(Ready_Components(), E_FAIL);
    CHECK_FAILED(SkillObject::Initialize(arg), E_FAIL);

    auto owner = Get_Owner();
    auto ownerTransform = owner ? owner->Get_Transform() : nullptr;
    CHECK_NULL(ownerTransform, E_FAIL);

    _delayElapsed = 0.f;
    _burstCount = 0;
    _isBurstFinished = false;
    _burstCenter = Vec3::Zero;

    // 타격 중심 위치 계산
    Resolve_BurstCenter();

    // 플레이어방향으로 약간 회전. 해야하나?
    if (_transformCom)
    {
        auto ownerPos = ownerTransform->Get_WorldPosition();
        auto ownerForward = ownerTransform->Get_WorldForward();

        const Vec3 spawnPos = ownerPos - ownerForward * 28.f + Vec3(0.f, 20.f, 0.f);

        _transformCom->Set_WorldPosition(spawnPos);
        _transformCom->LookAt(ownerTransform->Get_WorldPosition());

        const Quat yawOffset = Quat::CreateFromAxisAngle(Vec3::Up, XMConvertToRadians(180.f));
        _transformCom->Add_LocalRotation(yawOffset);

   
        _transformCom->Set_LocalScale(0.0005f, 0.0005f, 0.0005f);
    }

    if (_collider)
    {
        _collider->Set_IsActive(false);
    }


    return S_OK;
}

void Skill_ShinsuSenju::Update(float timeDelta)
{
    SkillObject::Update(timeDelta);

    if (Is_Destroy() || _isBurstFinished)
        return;

    _delayElapsed += timeDelta;

    if (_delayElapsed < _initialFireDelay)
        return;

    while (_burstCount < _maxBurstCount)
    {
        const float nextFireTime = _initialFireDelay + (_burstInterval * static_cast<float>(_burstCount));

        if (_delayElapsed + 0.0001f < nextFireTime)
            break;

        Fire_NextArm();
    }

    if (_burstCount >= _maxBurstCount)
    {
        _isBurstFinished = true;
        //Set_Destroy(true);
    }
}

void Skill_ShinsuSenju::Late_Update(float timeDelta)
{
    SkillObject::Late_Update(timeDelta);

    if (Is_Destroy())
        return;

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
}

HRESULT Skill_ShinsuSenju::Render()
{
    GameObject::Render();

    if (!_shader || !_model)
        return S_OK;

    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);

    const Vec4 outlineColor = Vec4(0.04f, 0.05f, 0.08f, 1.f);
    const float outlineThickness = 0.0035f;

    CHECK_FAILED(_shader->Bind_RawValue("g_OutlineColor", &outlineColor, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shader->Bind_RawValue("g_OutlineThickness", &outlineThickness, sizeof(float)), E_FAIL);

    const size_t numMeshes = _model->Get_NumMeshes();

    for (size_t i = 0; i < numMeshes; ++i)
    {
        CHECK_FAILED(_model->Bind_BoneMatrices(_shader, "g_BoneMatrices"), E_FAIL);
        _model->Bind_Material(_shader, "g_DiffuseTexture", i, EMaterialTextureSlot::BaseColor, 0);

        CHECK_FAILED(_shader->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(_model->Render(i), E_FAIL);
    }

    return S_OK;
}

HRESULT Skill_ShinsuSenju::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXANIMMESH, _shader), E_FAIL);

    const uint32 modelKey = static_cast<uint32>(std::hash<string>{}("ShinsuSenju"));
    CHECK_FAILED(Add_Component(modelKey, _model), E_FAIL);

    return S_OK;
}

HRESULT Skill_ShinsuSenju::Bind_ShaderResources()
{
    CHECK_NULL(_shader, E_FAIL);
    CHECK_NULL(_transformCom, E_FAIL);

    CHECK_FAILED(_shader->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    return S_OK;
}

void Skill_ShinsuSenju::Resolve_BurstCenter()
{
    auto owner = Get_Owner();
    auto ownerTransform = owner ? owner->Get_Transform() : nullptr;
    if (!ownerTransform)
        return;

    Vec3 forward = ownerTransform->Get_WorldForward();
    forward.y = 0.f;
    forward = Utils::Safe_Normalize(forward, Vec3::Forward);

    _burstCenter = ownerTransform->Get_WorldPosition() + forward * _forwardRange;
}

Vec3 Skill_ShinsuSenju::Build_ArmImpactPoint(int32 burstIndex)
{
    auto owner = Get_Owner();
    auto ownerTransform = owner ? owner->Get_Transform() : nullptr;
    if (!ownerTransform)
        return _burstCenter;

    static const Vec2 kPattern[5] =
    {
        Vec2(-0.60f, -0.25f),
        Vec2(-0.30f,  0.35f),
        Vec2( 0.00f,  0.00f),
        Vec2( 0.30f, -0.35f),
        Vec2( 0.60f,  0.25f),
    };

    const int32 safeIndex = (burstIndex < 0) ? 0 : ((burstIndex >= 5) ? 4 : burstIndex);
    const Vec2 pattern = kPattern[safeIndex];

    Vec3 right = ownerTransform->Get_WorldRight();
    right.y = 0.f;
    right = Utils::Safe_Normalize(right, Vec3::Right);

    Vec3 forward = ownerTransform->Get_WorldForward();
    forward.y = 0.f;
    forward = Utils::Safe_Normalize(forward, Vec3::Forward);

    Vec3 samplePosition = _burstCenter;
    samplePosition += right * (_impactRadius * pattern.x);
    samplePosition += forward * (_impactRadius * pattern.y);

    Vec3 groundPoint = samplePosition;
    if (!Resolve_GroundPoint(samplePosition, groundPoint))
    {
        return samplePosition;
    }

    return groundPoint;
}

void Skill_ShinsuSenju::Fire_NextArm()
{
    const Vec3 spawnPoint = Build_ArmSpawnPoint(_burstCount);
    const Vec3 impactPoint = Build_ArmImpactPoint(_burstCount);

    Spawn_Arm(spawnPoint, impactPoint);
    ++_burstCount;
}

bool Skill_ShinsuSenju::Resolve_GroundPoint(const Vec3& samplePosition, Vec3& outGroundPoint) const
{
    vector<MovementComponent::FCollisionModelInstance> walkableModels;
    vector<MovementComponent::FCollisionModelInstance> wallModels;

    GAME->Query_ActiveCollisionProxy(samplePosition, walkableModels, wallModels);

    const Vec3 rayOrigin = samplePosition + Vec3(0.f, 80.f, 0.f);
    const Ray downRay(rayOrigin, Vec3(0.f, -1.f, 0.f));

    bool found = false;
    float bestHeight = -FLT_MAX;
    Vec3 bestPoint = samplePosition;

    for (const auto& collision : walkableModels)
    {
        if (!collision.model)
            continue;

        if (collision.hasWorldBounds)
        {
            float boundsHitDist = 0.f;
            if (!downRay.Intersects(collision.worldBounds, boundsHitDist))
                continue;
        }

        float hitDist = 0.f;
        Vec3 hitPoint = Vec3::Zero;
        Vec3 hitNormal = Vec3::Up;

        if (!collision.model->Raycast(downRay, collision.worldMatrix, hitDist, hitPoint, hitNormal))
            continue;

        Vec3 surfaceNormal = Utils::Safe_Normalize(hitNormal, Vec3::Up);
        if (surfaceNormal.Dot(Vec3::Up) < 0.f)
            surfaceNormal *= -1.f;

        if (surfaceNormal.Dot(Vec3::Up) < 0.35f)
            continue;

        if (hitPoint.y > bestHeight)
        {
            bestHeight = hitPoint.y;
            bestPoint = hitPoint;
            found = true;
        }
    }

    outGroundPoint = bestPoint;
    return found;
}

Vec3 Skill_ShinsuSenju::Build_ArmSpawnPoint(int32 burstIndex) const
{
    if (!_transformCom)
        return Vec3::Zero;

    const Vec2 pattern = Get_ArmSpawnPattern(burstIndex);

    Vec3 right = _transformCom->Get_WorldRight();
    right = Utils::Safe_Normalize(right, Vec3::Right);

    Vec3 up = _transformCom->Get_WorldUp();
    up = Utils::Safe_Normalize(up, Vec3::Up);

    Vec3 spawnPoint = _transformCom->Get_WorldPosition();
    spawnPoint += right * (_armSpawnRadius * pattern.x);
    spawnPoint += up * (_armSpawnHeight * pattern.y);

    return spawnPoint;
}

Vec2 Skill_ShinsuSenju::Get_ArmSpawnPattern(int32 burstIndex) const
{
     static const Vec2 kPattern[5] =
    {
        Vec2(-1.0f, -0.2f),
        Vec2(-0.8f,  0.6f),
        Vec2( 0.0f,  1.0f),
        Vec2( 0.8f,  0.6f),
        Vec2( 1.0f, -0.2f),
    };

    const int32 safeIndex = (burstIndex < 0) ? 0 : ((burstIndex >= 5) ? 4 : burstIndex);
    return kPattern[safeIndex];
}

void Skill_ShinsuSenju::Spawn_Arm(const Vec3& spawnPoint, const Vec3& impactPoint)
{
    {
        FDebugLineDesc armPathLineDesc{}; 
        armPathLineDesc.start = spawnPoint;
        armPathLineDesc.end = impactPoint;
        armPathLineDesc.style.color = Color(0.f, 1.f, 0.f, 1.f);
        armPathLineDesc.style.duration = 15.f;
        armPathLineDesc.style.depthEnabled = true;
        GAME->Draw_DebugLine(armPathLineDesc);

        FDebugSphereDesc impactPointSphereDesc{};
        impactPointSphereDesc.center = impactPoint;
        impactPointSphereDesc.radius = 0.6f;
        impactPointSphereDesc.style.color = Color(1.f, 0.2f, 0.2f, 1.f);
        impactPointSphereDesc.style.duration = 15.f;
        impactPointSphereDesc.style.depthEnabled = true;
        GAME->Draw_DebugSphere(impactPointSphereDesc);

        FDebugSphereDesc spawnPointSphereDesc{}; 
        spawnPointSphereDesc.center = spawnPoint;
        spawnPointSphereDesc.radius = 0.45f;
        spawnPointSphereDesc.style.color = Color(0.2f, 0.8f, 1.f, 1.f);
        spawnPointSphereDesc.style.duration = 15.f;
        spawnPointSphereDesc.style.depthEnabled = true;
        GAME->Draw_DebugSphere(spawnPointSphereDesc);
    }

    auto owner = Get_Owner();
    if (!owner)
        return;

    Vec3 launchDirection = impactPoint - spawnPoint;
    launchDirection = Utils::Safe_Normalize(launchDirection, Vec3(0.f, -1.f, 0.f));

    SkillObject_Projectile::FProjectileSkillDesc desc{};
    desc.ownerObject = owner;

    desc.spawnPosition = spawnPoint;


    {
        const Quat finalRotation = Build_ShinsuSenjuArmSpawnRotation(launchDirection);
        const Vec3 finalEulerRadians = finalRotation.ToEuler();

        desc.spawnRotation = Vec3(
            XMConvertToDegrees(finalEulerRadians.x),
            XMConvertToDegrees(finalEulerRadians.y),
            XMConvertToDegrees(finalEulerRadians.z));
    }

    desc.scale = Vec3(1.f);

    desc.direction = launchDirection;
    desc.speed = _armSpeed;
    desc.maxDistance = 200.f;
    desc.lifetime = 10.f;

    desc.colliderType     = Protocol::COMPONENT_TYPE_COLLIDER_OBB;
    desc.colliderExtents  = Vec3(18000, 4000, 4000);
    desc.collisionPreset  = Collision_Preset::Player_Attack;

    desc.startAttached = true;
    desc.useDirectionLookAt = false;

    desc.hasTargetPoint = true;
    desc.targetPoint = impactPoint;

    auto spawned = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        Protocol::OBJECT_TYPE_SHINSUSENJU_ARM,
        GAME->Current_Level(),
        TEXT("Layer_Skill"),
        &desc);

    auto arm = dynamic_pointer_cast<SkillObject_Projectile>(spawned);
    if (!arm)
        return;



    arm->Set_Owner(owner);
    arm->Launch(launchDirection);
}


Shared<GameObject> Skill_ShinsuSenju::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_ShinsuSenju>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_ShinsuSenju");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_ShinsuSenju::Clone(void* arg)
{
    auto clone = make_shared<Skill_ShinsuSenju>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_ShinsuSenju");

        return nullptr;
    }

    return clone;
}

void Skill_ShinsuSenju::Free()
{
    SkillObject::Free();
}
