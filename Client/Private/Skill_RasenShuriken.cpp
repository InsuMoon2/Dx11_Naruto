#include "pch.h"
#include "Skill_RasenShuriken.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_RasenShuriken, Protocol::OBJECT_TYPE_SKILL_RASENSHURIKEN, "SkillSpawn");

Skill_RasenShuriken::Skill_RasenShuriken(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject_Projectile(device, context)
{
}

Skill_RasenShuriken::Skill_RasenShuriken(const Skill_RasenShuriken& rhs)
    : SkillObject_Projectile(rhs)
{
}

HRESULT Skill_RasenShuriken::Initialize_Prototype()
{
    return SkillObject_Projectile::Initialize_Prototype();
}

HRESULT Skill_RasenShuriken::Initialize(void* arg)
{
    auto* srcDesc = static_cast<SkillObject_Projectile::FProjectileSkillDesc*>(arg);
    if (!srcDesc)
        return E_FAIL;

    SkillObject_Projectile::FProjectileSkillDesc desc = *srcDesc;

    desc.colliderType = Protocol::COMPONENT_TYPE_COLLIDER_SPHERE;
    desc.colliderRadius = _colliderRadius;
    desc.speed = _speed;
    desc.maxDistance = _maxDistance;
    desc.lifetime = _lifetime;

    desc.useGravity = false;
    desc.scale = Vec3(1.f, 1.f, 1.f);

    CHECK_FAILED(SkillObject_Projectile::Initialize(&desc), E_FAIL);

    return S_OK;
}

void Skill_RasenShuriken::Update(float timeDelta)
{
    SkillObject_Projectile::Update(timeDelta);

    if (Is_Destroy())
        return;



}

Shared<GameObject> Skill_RasenShuriken::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_RasenShuriken>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_RasenShuriken");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_RasenShuriken::Clone(void* arg)
{
    auto clone = make_shared<Skill_RasenShuriken>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_RasenShuriken");

        return nullptr;
    }

    return clone;
}

void Skill_RasenShuriken::Free()
{
    SkillObject_Projectile::Free();
}
