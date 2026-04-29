#include "pch.h"
#include "SkillObject.h"
#include "Model.h"
#include "Shader.h"
#include "Transform.h"
#include "GameObject.h"
#include "GameObject_Factory.h"
#include "Collider.h"
#include "Bounding_AABB.h"
#include "Bounding_OBB.h"
#include "Bounding_Sphere.h"
#include "MyPlayer.h"
#include "EffectComponent.h"
#include "AttachedEffectObject.h"

REGISTER_GAMEOBJECT(SkillObject, Protocol::OBJECT_TYPE_SKILL_OBJECT)

// SpawnAttack/투사체가 현재 바라보는 방향을 피격 launch 방향으로 넘기기 위해 호출한다.
// owner 중심 기준 대신 실제 공격체 forward를 쓰면 근접 body blow에서도 밀림 방향이 안정적이다.
static Vec3 Resolve_SkillHitDamageDirection(const Shared<Transform>& transform)
{
    if (!transform)
        return Vec3::Zero;

    Vec3 forward = transform->Get_WorldForward();
    forward.y = 0.f;

    return Utils::Safe_Normalize(forward, Vec3::Zero);
}

SkillObject::SkillObject(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

SkillObject::SkillObject(const SkillObject& rhs)
    : GameObject(rhs)
    , _lifetime(rhs._lifetime)                 
    , _elapsedTime(0.f)                        
    , _ownerSkillId(rhs._ownerSkillId)         
    , _collisionPreset(rhs._collisionPreset)
    , _hitCount(rhs._hitCount) 
    , _maxHitCount(rhs._maxHitCount)
    , _hitInterval(rhs._hitInterval)
    , _hitLaunchForce(rhs._hitLaunchForce)
    , _colliderRadius(rhs._colliderRadius)
    , _effectAssetName(rhs._effectAssetName)
    , _useHitReactionOverride(rhs._useHitReactionOverride)
    , _hitReactionType(rhs._hitReactionType)
    , _useLaunchOverride(rhs._useLaunchOverride)
    , _launchPower(rhs._launchPower)
    , _launchUp(rhs._launchUp)
    , _useHitSoundOverride(rhs._useHitSoundOverride)
    , _hitSound(rhs._hitSound)
    , _hitSoundFile(rhs._hitSoundFile)
{
}

HRESULT SkillObject::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT SkillObject::Initialize(void* arg)
{
    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FSkillObjectDesc*>(arg);

    _elapsedTime = 0.f;

    if (desc)
    {
        if (desc->lifetime > 0.f)
            _lifetime = desc->lifetime;

        _ownerSkillId = desc->ownerSkillId;
        _collisionPreset = desc->collisionPreset;
        _effectAssetName = desc->effectAssetName;
        _attachOffset = desc->attachOffset;
        _useHitReactionOverride = desc->useHitReactionOverride;
        _hitReactionType = desc->hitReactionType;
        _useLaunchOverride = desc->useLaunchOverride;
        _launchPower = desc->launchPower;
        _launchUp = desc->launchUp;
        _useHitSoundOverride = desc->useHitSoundOverride;
        _hitSound = desc->hitSound;
        _hitSoundFile = desc->hitSoundFile;

        if (_transformCom)
        {
            _transformCom->Set_LocalPosition(desc->spawnPosition);
            _transformCom->Set_WorldRotation(desc->spawnRotation.x, desc->spawnRotation.y, desc->spawnRotation.z);
            _transformCom->Set_LocalScale(desc->scale);

            if (desc->useDirectionLookAt && desc->direction.LengthSquared() > 0.001f)
            {
                const Vec3 lookTarget = desc->spawnPosition + desc->direction;
                _transformCom->LookAt(lookTarget);
            }
        }

        CHECK_FAILED(Ready_Components(*desc), E_FAIL);

        if (desc->ownerObject)
            Set_Owner(desc->ownerObject);
    }

    else
    {
        LOG_INFO("음.. 이거 없이 넘기면 안되는데");
    }

    if (_collider)
        _collider->Set_CollisionPreset(_collisionPreset);

    return S_OK;
}

void SkillObject::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void SkillObject::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    if (_effectCom)
        _effectCom->Update(timeDelta);

    if (_lifetime > 0.f)
    {
        _elapsedTime += timeDelta;

        if (_elapsedTime >= _lifetime)
        {
            Set_Destroy(true);
        }
    }

    // 히트 쿨타임 관리
    for (auto& pair : _hitCooldowns)
    {
        if (pair.second > 0.f)
        {
            pair.second -= timeDelta;
        }
    }
}

void SkillObject::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    if (_effectCom)
        _effectCom->Late_Update(timeDelta);

    if (!Is_Destroy() && _collider)
    {
        _collider->Update_Collider(_transformCom->Get_WorldMatrix());

        GAME->Add_Collider(_collider);
    }
}

void SkillObject::Sync_AttachedTransform(const Matrix& boneWorldMatrix)
{
    if (!_transformCom)
        return;

    Matrix tempMatrix = boneWorldMatrix;
    Vec3 worldPos, worldScale;
    Quat worldQuat;

    if (tempMatrix.Decompose(worldScale, worldQuat, worldPos))
    {
        const Matrix rotationMat = Matrix::CreateFromQuaternion(worldQuat);
        const Vec3 worldOffset = Vec3::TransformNormal(_attachOffset, rotationMat);

        _transformCom->Set_WorldPosition(worldPos + worldOffset);
        _transformCom->Set_WorldRotation(worldQuat);
        return;
    }

    _transformCom->Set_WorldPosition(tempMatrix.Translation() + _attachOffset);
}

Shared<GameObject> SkillObject::Spawn_Effect_Once(const string& effectAssetName, const Vec3& worldPosition,
    const Vec3 worldScale)
{
    AttachedEffectObject::FAttachedEffectObjectDesc desc{};
    desc.effectAssetName = effectAssetName;
    desc.loopOverride    = false;

     auto spawned = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        Protocol::OBJECT_TYPE_ATTACHED_EFFECT,
        GAME->Current_Level(),
        TEXT("Layer_Skill"),
        &desc);

    if (!spawned)
        return nullptr;

    if (auto transform = spawned->Get_Transform())
    {
        transform->Set_WorldPosition(worldPosition);
        transform->Set_LocalScale(worldScale);
    }

    return spawned;
}

HRESULT SkillObject::Ready_Components(const FSkillObjectDesc& desc)
{
    if (desc.colliderType == Protocol::COMPONENT_TYPE_COLLIDER_SPHERE)
    {
        Bounding_Sphere::FBoundingSphereDesc sphereDesc{};
        sphereDesc.radius = desc.colliderRadius;

        CHECK_FAILED(Add_Component(desc.colliderType, _collider, &sphereDesc), E_FAIL);
    }
    else if (desc.colliderType == Protocol::COMPONENT_TYPE_COLLIDER_AABB)
    {
        Bounding_AABB::FBoundingAABBDesc boxDesc{};
        boxDesc.extents = desc.colliderExtents;

        CHECK_FAILED(Add_Component(desc.colliderType, _collider, &boxDesc), E_FAIL);
    }
    else if (desc.colliderType == Protocol::COMPONENT_TYPE_COLLIDER_OBB)
    {
        Bounding_OBB::FBoundingOBBDesc boxDesc{};
        boxDesc.extents = desc.colliderExtents;
        boxDesc.radians = desc.spawnRotation;

        CHECK_FAILED(Add_Component(desc.colliderType, _collider, &boxDesc), E_FAIL);
    }
    else
    {
        return E_FAIL;
    }

    CHECK_NULL(_collider, E_FAIL);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_EFFECT, _effectCom), E_FAIL);


    return S_OK;
}

bool SkillObject::Apply_Skill_Hit(Character* hitted, float damage, float launchForce, float launchUp)
{
    CHECK_NULL(hitted, false);

    auto owner = Get_Owner();
    if (!owner)
        return false;

    if (hitted == owner.get())
        return false;

    FDamageEvent damageEvent{};
    const float finalLaunchPower = _useLaunchOverride ? _launchPower : launchForce;
    const float finalLaunchUp = _useLaunchOverride ? _launchUp : launchUp;

    damageEvent.damage = damage;
    damageEvent.damageCauser = owner;
    damageEvent.damageSourceType = Get_ObjectType();
    damageEvent.launchPower = finalLaunchPower;
    damageEvent.launchUp = finalLaunchUp;
    damageEvent.hitSound = _useHitSoundOverride ? _hitSound : 0;
    damageEvent.hitSoundFile = _useHitSoundOverride ? _hitSoundFile : "";
    damageEvent.forceHitRestart = true;

    const Vec3 damageDir = Resolve_SkillHitDamageDirection(_transformCom);
    if (damageDir.LengthSquared() > FLT_EPSILON)
    {
        damageEvent.damageDir = damageDir;
        damageEvent.hasCustomDir = true;
    }

    if (_useHitReactionOverride)
        damageEvent.hitReactionType = _hitReactionType;
    else if (finalLaunchPower > 0.f && finalLaunchUp > 0.f)
        damageEvent.hitReactionType = EHitReactionType::Launch;

    else if (finalLaunchPower > 0.f)
        damageEvent.hitReactionType = EHitReactionType::BlowOff;

    else
        damageEvent.hitReactionType = EHitReactionType::Stagger;

    hitted->TakeDamage(damageEvent);

    if (auto myPlayer = dynamic_pointer_cast<MyPlayer>(owner))
        myPlayer->Add_ComboHit();

    return true;
}

Shared<GameObject> SkillObject::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<SkillObject>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : SkillObject");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> SkillObject::Clone(void* arg)
{
    auto clone = make_shared<SkillObject>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : SkillObject");

        return nullptr;
    }

    return clone;
}

void SkillObject::Free()
{
    GameObject::Free();
}
