#include "pch.h"
#include "Skill_RasenShuriken.h"
#include "GameObject_Factory.h"
#include "Collider.h"
#include "GameObject.h"
#include "Character.h"
#include "Bounding_Sphere.h"
#include "MyPlayer.h"
#include "EffectComponent.h"
#include "Skill_RasenShuriken_Hit.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_RasenShuriken, Protocol::OBJECT_TYPE_SKILL_RASENSHURIKEN, "SkillSpawn");

Skill_RasenShuriken::Skill_RasenShuriken(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject_Projectile(device, context)
{
}

Skill_RasenShuriken::Skill_RasenShuriken(const Skill_RasenShuriken& rhs)
    : SkillObject_Projectile(rhs)
    , _hasExploded(rhs._hasExploded)
{
}

HRESULT Skill_RasenShuriken::Initialize_Prototype()
{
    _speed = 28.f;
    _maxDistance = 7.f;
    _lifetime = 2.5f;
    _colliderRadius = 1.35f;

    _collisionPreset = Collision_Preset::Player_Attack;

    _isMoving = false;

    return SkillObject_Projectile::Initialize_Prototype();
}

HRESULT Skill_RasenShuriken::Initialize(void* arg)
{
    CHECK_FAILED(SkillObject_Projectile::Initialize(arg), E_FAIL);

    _isMoving = false;
    _hasExploded = false;

    EffectComponent::FPlayDesc playDesc{};
    playDesc.effectAssetName = "RasenganShuriken";

    CHECK_FAILED(_effectCom->Play_Effect(playDesc), E_FAIL);

    return S_OK;
}

void Skill_RasenShuriken::Update(float timeDelta)
{
    SkillObject_Projectile::Update(timeDelta);

    if (Is_Destroy())
    {
        Explode_RasenShuriken();
        return;
    }
}

void Skill_RasenShuriken::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
     if (Is_Destroy())
        return;

    auto character = Find_HitCharacter(other);
    if (!character)
        return;

    auto otherOwner = other->Get_Owner();
    CHECK_NULL(otherOwner);

    // 1타 적용
    Apply_Skill_Hit(character, 10.f, 2.5f, 0.f); 
    auto targetTransform = otherOwner->Get_Transform();
    if (targetTransform)
    {
        Vec3 explosionPosition = targetTransform->Get_WorldPosition();
        Explode_RasenShuriken(&explosionPosition);
    }
    else
    {
        Explode_RasenShuriken();
    }
}

void Skill_RasenShuriken::OnStayOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject_Projectile::OnStayOverlap(self, other);

   
}

void Skill_RasenShuriken::OnEndOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject_Projectile::OnEndOverlap(self, other);

    
}

Character* Skill_RasenShuriken::Find_HitCharacter(Shared<Collider> other)
{
    if (!other || Is_Destroy())
        return nullptr;

    auto otherOwner = other->Get_Owner();
    if (otherOwner == nullptr)
        return nullptr;

    if (otherOwner == Get_Owner())
        return nullptr;

    return dynamic_cast<Character*>(otherOwner.get());
}

void Skill_RasenShuriken::Explode_RasenShuriken(const Vec3* overrideExplosionPosition)
{
    if (_hasExploded)
        return;

    _hasExploded = true;
    const Vec3 explosionPosition =
        overrideExplosionPosition
        ? *overrideExplosionPosition
        : (_transformCom ? _transformCom->Get_WorldPosition() : Vec3::Zero);

    Skill_RasenShuriken_Hit::FHitDesc desc{};
    desc.damageCauser = Get_Owner();
    desc.spawnPosition = explosionPosition;
    desc.collisionPreset = Collision_Preset::Player_Attack;

    GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        Protocol::OBJECT_TYPE_SKILL_RASENSHURIKEN_HIT,
        GAME->Current_Level(),
        TEXT("Layer_Skill"), &desc);

    Set_Destroy(true);
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
