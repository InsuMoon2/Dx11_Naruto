#include "pch.h"
#include "Skill_ShinsuSenju_Arm.h"
#include "GameObject_Factory.h"
#include "GameObject.h"
#include "Collider.h"
#include "Character.h"
#include "MeshDebrisObject.h"
#include "Shader.h"
#include "Model.h"
#include "Skill_ShinsuSenju_Impact.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_ShinsuSenju_Arm, Protocol::OBJECT_TYPE_SHINSUSENJU_ARM, "SkillSpawn");

Skill_ShinsuSenju_Arm::Skill_ShinsuSenju_Arm(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject_Projectile(device, context)
{
}

Skill_ShinsuSenju_Arm::Skill_ShinsuSenju_Arm(const Skill_ShinsuSenju_Arm& rhs)
    : SkillObject_Projectile(rhs)
    , _shader(rhs._shader)
    , _model(rhs._model)
    , _isStop(false)
{

}

HRESULT Skill_ShinsuSenju_Arm::Initialize_Prototype()
{
    _lifetime = 15.f;

    _colliderRadius = 10.2f;
    _collisionPreset = Collision_Preset::Player_Attack;

    _speed = 55.f;
    _maxDistance = 200.f;
    _maxHitCount = 1;
    _hitLaunchForce = 2.5f;

    _damage = 20.f;

    return SkillObject_Projectile::Initialize_Prototype();
}

HRESULT Skill_ShinsuSenju_Arm::Initialize(void* arg)
{
    CHECK_FAILED(Ready_Components(), E_FAIL);
    CHECK_FAILED(SkillObject_Projectile::Initialize(arg), E_FAIL);

    if (_transformCom)
    {
        _transformCom->Set_LocalScale(0.0005f, 0.0005f, 0.0005f);

        
    }
    auto* desc = static_cast<FProjectileSkillDesc*>(arg);
    if (desc)
    {
        _targetPoint = desc->targetPoint;
        _hasTargetPoint = desc->hasTargetPoint;

    }

    _collider->Set_IsActive(true);

    return S_OK;
}

void Skill_ShinsuSenju_Arm::Update(float timeDelta)
{
    SkillObject_Projectile::Update(timeDelta);

    if (Is_Destroy())
        return;

    if (_hasTargetPoint && _isMoving && _transformCom)
    {
        const Vec3 currentPos = _transformCom->Get_WorldPosition();
        const float remainingDist = Vec3::Distance(currentPos, _targetPoint);

        // 목표지점 도달
        if (remainingDist <= _stopDistance)
        {
            _isMoving = false;
            _isStop = true;

            if (_collider)
                _collider->Set_IsActive(false);

            Spawn_Particle("SmallRock", 10);
        }
    }


}

void Skill_ShinsuSenju_Arm::Late_Update(float timeDelta)
{
    SkillObject_Projectile::Late_Update(timeDelta);

    if (Is_Destroy())
        return;

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
}

HRESULT Skill_ShinsuSenju_Arm::Render()
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

void Skill_ShinsuSenju_Arm::Apply_InitialRotation(const Vec3& launchDirection)
{
    if (!_transformCom)
        return;

    const Vec3 currentPos = _transformCom->Get_WorldPosition();
    const Vec3 targetPos = currentPos + Utils::Safe_Normalize(launchDirection, Vec3(0.f, -1.f, 0.f)) * 10.f;

    _transformCom->LookAt(targetPos);

    const Quat axisCorr = Quat::CreateFromAxisAngle(Vec3::Up, XMConvertToRadians(270.f));
    _transformCom->Add_LocalRotation(axisCorr);

    const Quat yaw = Quat::CreateFromAxisAngle(Vec3::Right, XMConvertToRadians(-60.f));
    _transformCom->Add_WorldRotation(yaw);
}

void Skill_ShinsuSenju_Arm::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    if (!other)
        return;

    auto otherOwner = other->Get_Owner();
    if (!otherOwner || otherOwner == Get_Owner())
        return;

    auto character = dynamic_cast<Character*>(otherOwner.get());
    CHECK_NULL(character);

    if (!Apply_Skill_Hit(character, _damage, _hitLaunchForce, 0.f))
        return;


}

HRESULT Skill_ShinsuSenju_Arm::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXANIMMESH, _shader), E_FAIL);

    const uint32 modelKey = static_cast<uint32>(std::hash<string>{}("ShinsuSenju_Arm_Long"));
    CHECK_FAILED(Add_Component(modelKey, _model), E_FAIL);

    return S_OK;
}

HRESULT Skill_ShinsuSenju_Arm::Bind_ShaderResources()
{
    CHECK_NULL(_shader, E_FAIL);
    CHECK_NULL(_transformCom, E_FAIL);

    CHECK_FAILED(_shader->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    return S_OK;
}

void Skill_ShinsuSenju_Arm::Spawn_Particle(const string& assetName, int32 spawnIndex)
{
    const int32 safeCount = (spawnIndex <= 0) ? 1 : spawnIndex;

    for (int32 i = 0; i < safeCount; ++i)
    {
        MeshDebrisObject::FMeshDebrisDesc debrisDesc{};

        debrisDesc.effectAssetName = assetName;
        debrisDesc.position = _targetPoint;

        debrisDesc.spawnRotation = Vec3(
            Utils::RandomRange(0.f, 360.f),
            Utils::RandomRange(0.f, 360.f),
            Utils::RandomRange(0.f, 360.f));

        debrisDesc.spawnScale = Vec3(1.f, 1.f, 1.f);

        const float randomScale = Utils::RandomRange(0.10f, 0.18f);
        debrisDesc.effectLocalScale = Vec3(randomScale, randomScale, randomScale);

        debrisDesc.initialVelocity = Vec3(
            Utils::RandomRange(-3.5f, 3.5f),
            Utils::RandomRange(7.f, 10.f),
            Utils::RandomRange(-3.5f, 3.5f));

        debrisDesc.gravity = -24.f;

        debrisDesc.angularVelocityDeg = Vec3(
            Utils::RandomRange(-360.f, 360.f),
            Utils::RandomRange(-360.f, 360.f),
            Utils::RandomRange(-360.f, 360.f));

        debrisDesc.lifetime = Utils::RandomRange(0.8f, 1.2f);
        debrisDesc.groundY = _targetPoint.y;

        debrisDesc.destroyOnGroundHit = false;
        debrisDesc.stopOnGroundHit = true;

        GAME->Clone_And_Add_GameObject(
            ETOI(ELevelType::Static),
            Protocol::OBJECT_TYPE_MESH_DEBRIS,
            GAME->Current_Level(),
            TEXT("Layer_Effect"),
            &debrisDesc);
    }

}

Shared<GameObject> Skill_ShinsuSenju_Arm::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_ShinsuSenju_Arm>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_ShinsuSenju_Arm");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_ShinsuSenju_Arm::Clone(void* arg)
{
    auto clone = make_shared<Skill_ShinsuSenju_Arm>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_ShinsuSenju_Arm");

        return nullptr;
    }

    return clone;
}

void Skill_ShinsuSenju_Arm::Free()
{
    SkillObject_Projectile::Free();
}
