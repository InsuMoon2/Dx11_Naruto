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
    , _isMoving(rhs._isMoving)             
    , _speed(rhs._speed)                   
    , _maxDistance(rhs._maxDistance)       
    , _hitCount(rhs._hitCount)             
    , _maxHitCount(rhs._maxHitCount)       
    , _hitInterval(rhs._hitInterval)       
    , _hitLaunchForce(rhs._hitLaunchForce) 
    , _colliderRadius(rhs._colliderRadius) 
{
}

HRESULT SkillObject_Projectile::Initialize_Prototype()
{
    return SkillObject::Initialize_Prototype();
}

HRESULT SkillObject_Projectile::Initialize(void* arg)
{
	CHECK_FAILED(SkillObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FProjectileSkillDesc*>(arg);

	if (desc)
	{
		if (desc->speed > 0.f)
			_speed = desc->speed;

		if (desc->maxDistance > 0.f)
			_maxDistance = desc->maxDistance;

		if (desc->colliderRadius > 0.f)
			_colliderRadius = desc->colliderRadius;

		_collisionPreset = desc->collisionPreset;

		_isMoving = !desc->startAttached;
	}
	else
	{
		// 프로토타입 상태 유지
	}

	ProjectileComponent::FProjectileDesc projDesc;
	if (desc)
	{
		projDesc.direction = desc->direction;
		projDesc.useGravity = desc->useGravity;
	}
	else
	{
		projDesc.direction = Vec3::Forward;
		projDesc.useGravity = false;
	}

	projDesc.speed = _speed;
	projDesc.maxDistance = _maxDistance;
	projDesc.maxLifetime = _lifetime;

	CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_PROJECTILE, _projectile, &projDesc), E_FAIL);

	return S_OK;
}

void SkillObject_Projectile::Update(float timeDelta)
{
    // 위치 갱신
    if (_isMoving && _projectile)
        _projectile->Update_Projectile(timeDelta);

    // 움직일 때에만 LifeTime이 소비되도록
	if (_isMoving)
		SkillObject::Update(timeDelta);
	else
		GameObject::Update(timeDelta);

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
