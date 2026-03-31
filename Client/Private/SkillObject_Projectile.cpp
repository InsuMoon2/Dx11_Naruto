#include "pch.h"
#include "SkillObject_Projectile.h"
#include "ProjectileComponent.h"
#include "GameObject_Factory.h"

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
    CHECK_FAILED(SkillObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FProjectileSkillDesc*>(arg);
    if (!desc)
        return E_FAIL;

    ProjectileComponent::FProjectileDesc projDesc;
    projDesc.direction = desc->direction;
    projDesc.speed = desc->speed;
    projDesc.maxDistance = desc->maxDistance;
    projDesc.maxLifetime = desc->lifetime; 
    projDesc.useGravity = desc->useGravity;

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_PROJECTILE, _projectile, &projDesc), E_FAIL);

    return S_OK;
}

void SkillObject_Projectile::Update(float timeDelta)
{
    // 위치 갱신
    if (_projectile)
        _projectile->Update_Projectile(timeDelta);

    // 이후 라이프타임 체크
    SkillObject::Update(timeDelta);
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
