#include "pch.h"
#include "SkillObject_Projectile.h"
#include "ProjectileComponent.h"
#include "GameObject_Factory.h"
#include "Collider.h"
#include "Character.h"
#include "MovementComponent.h"

REGISTER_GAMEOBJECT(SkillObject_Projectile, Protocol::OBJECT_TYPE_SKILL_PROJECTILE)

SkillObject_Projectile::SkillObject_Projectile(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject(device, context)
{
}

SkillObject_Projectile::SkillObject_Projectile(const SkillObject_Projectile& rhs)
    : SkillObject(rhs)
{
}

HRESULT SkillObject_Projectile::Initialize_Prototype()
{
    return SkillObject::Initialize_Prototype();
}

HRESULT SkillObject_Projectile::Initialize(void* arg)
{
    auto* desc = static_cast<FProjectileSkillDesc*>(arg);
    if (!desc) return E_FAIL;

    _speed = desc->speed;
    _maxDistance = desc->maxDistance;
    _isMoving = !desc->startAttached;
    _colliderRadius = desc->colliderRadius;

    CHECK_FAILED(SkillObject::Initialize(arg), E_FAIL);

    ProjectileComponent::FProjectileDesc projDesc;
    projDesc.direction = desc->direction;
    projDesc.speed = _speed;
    projDesc.maxDistance = _maxDistance;
    projDesc.maxLifetime = _lifetime;
    projDesc.useGravity = desc->useGravity;

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_PROJECTILE, _projectile, &projDesc), E_FAIL);

    return S_OK;
}

void SkillObject_Projectile::Update(float timeDelta)
{
    // 위치 갱신
    if (_isMoving && _projectile)
        _projectile->Update_Projectile(timeDelta);

    // 이후 라이프타임 체크
    SkillObject::Update(timeDelta);

    // 히트 쿨타임 관리
    for (auto& pair : _hitCooldowns)
    {
        if (pair.second > 0.f)
        {
            pair.second -= timeDelta;
        }
    }
}

void SkillObject_Projectile::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnBeginOverlap(self, other);

    if (!other || Is_Destroy())
        return;

    auto otherOwner = other->Get_Owner();
    if (!otherOwner || otherOwner == Get_Owner())
        return;

    // 히트 쿨타임 처리
    if (_hitCooldowns[otherOwner.get()] > 0.f)
        return;

    _hitCount++;
    _hitCooldowns[otherOwner.get()] = _hitInterval;

    // 데미지 처리
    auto character = dynamic_cast<Character*>(otherOwner.get());
    if (character)
    {
        character->TakeDamage(FDamageEvent{ 10.f, nullptr });

        if (_hitLaunchForce > 0.f)
        {
            auto moveComp = character->Get_Component<MovementComponent>();
            if (moveComp)
                moveComp->Launch(Vec3(0, 1, 0) * _hitLaunchForce, false, true);
        }
    }

    // 최대 카운트 도달 or LifeTime으로 처리
    if (_hitCount >= _maxHitCount)
    {
        Set_Destroy(true);
    }
}

void SkillObject_Projectile::Launch(const Vec3& direction)
{
    if (_isMoving)
        return;

    if (_projectile)
        _projectile->Set_Direction(direction);

    if (_collider)
        _collider->Set_IsActive(true); // 발사 시 콜라이더 활성화
    _isMoving = true;
}

void SkillObject_Projectile::Sync_AttachedTransform(const Matrix& boneWorldMatrix)
{
    if (IsLaunched() || !_transformCom)
        return;

    Matrix tempMatrix = boneWorldMatrix;

    Vec3 worldPos, worldScale;
    Quat worldQuat;

    if (tempMatrix.Decompose(worldScale, worldQuat, worldPos))
    {
        _transformCom->Set_WorldPosition(worldPos);
        _transformCom->Set_WorldRotation(worldQuat);
    }
}

Shared<GameObject> SkillObject_Projectile::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<SkillObject_Projectile>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : SkillObject_Projectile");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> SkillObject_Projectile::Clone(void* arg)
{
    auto clone = make_shared<SkillObject_Projectile>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : SkillObject_Projectile");

        return nullptr;
    }

    return clone;
}

void SkillObject_Projectile::Free()
{
    SkillObject::Free();
}
