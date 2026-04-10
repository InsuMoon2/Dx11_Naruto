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

REGISTER_GAMEOBJECT(SkillObject, Protocol::OBJECT_TYPE_SKILL_OBJECT)

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

        if (_transformCom)
        {
            _transformCom->Set_LocalPosition(desc->spawnPosition);
            _transformCom->Set_LocalRotation(desc->spawnRotation.x, desc->spawnRotation.y, desc->spawnRotation.z);
            _transformCom->Set_LocalScale(desc->scale);

            if (desc->direction.LengthSquared() > 0.001f)
            {
                const Vec3 lookTarget = desc->spawnPosition + desc->direction;
                _transformCom->LookAt(lookTarget);
            }
        }

        CHECK_FAILED(Ready_Components(*desc), E_FAIL);
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
    }
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
    damageEvent.damage = damage;
    damageEvent.damageCauser = owner;
    damageEvent.launchPower = launchForce;
    damageEvent.launchUp = launchUp;

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
