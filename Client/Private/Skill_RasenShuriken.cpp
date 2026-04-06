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
    _speed = 28.f;
    _maxDistance = 35.f;
    _lifetime = 5.5f;
    _maxHitCount = 5;
    _hitInterval = 0.12f;
    _hitLaunchForce = 250.f;
    _colliderRadius = 1.35f;

    _collisionPreset = Collision_Preset::Player_Attack;

    _isMoving = false;

    return SkillObject_Projectile::Initialize_Prototype();
}

HRESULT Skill_RasenShuriken::Initialize(void* arg)
{
    CHECK_FAILED(SkillObject_Projectile::Initialize(arg), E_FAIL);

    _isMoving = false;

    return S_OK;
}

void Skill_RasenShuriken::Update(float timeDelta)
{
    SkillObject_Projectile::Update(timeDelta);

    if (Is_Destroy())
        return;

}

void Skill_RasenShuriken::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject_Projectile::OnBeginOverlap(self, other);
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
