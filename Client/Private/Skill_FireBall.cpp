#include "pch.h"
#include "Skill_FireBall.h"
#include "GameObject_Factory.h"
#include "Collider.h"
#include "GameObject.h"
#include "Character.h"
#include "Bounding_Sphere.h"
#include "MyPlayer.h"
#include "EffectComponent.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_FireBall, Protocol::OBJECT_TYPE_SKILL_FIREBALL, "SkillSpawn");

Skill_FireBall::Skill_FireBall(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject_Projectile(device, context)
{
}

Skill_FireBall::Skill_FireBall(const Skill_FireBall& rhs)
    : SkillObject_Projectile(rhs)
    , _launchPower(rhs._launchPower)
    , _launchUp(rhs._launchUp)
{
}

HRESULT Skill_FireBall::Initialize_Prototype()
{
    _speed = 28.f;
    _maxDistance = 35.f;
    _lifetime = 5.5f;
    _maxHitCount = 1;
    _hitLaunchForce = 2.5f;
    _colliderRadius = 2.35f;

    _collisionPreset = Collision_Preset::Player_Attack;

    _isMoving = true;

    return SkillObject_Projectile::Initialize_Prototype();
}

HRESULT Skill_FireBall::Initialize(void* arg)
{
    CHECK_FAILED(SkillObject_Projectile::Initialize(arg), E_FAIL);

    _isMoving = true;

    EffectComponent::FPlayDesc playDesc{};
    playDesc.effectAssetName = "FireBall_Loop";

    CHECK_FAILED(_effectCom->Play_Effect(playDesc), E_FAIL);

    return S_OK;
}

void Skill_FireBall::Update(float timeDelta)
{
    SkillObject_Projectile::Update(timeDelta);

    if (Is_Destroy())
        return;

    
}

void Skill_FireBall::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
     SkillObject::OnBeginOverlap(self, other);

    if (Is_Destroy())
        return;

    if (!other)
        return;

    auto otherOwner = other->Get_Owner();
    if (!otherOwner)
        return;

    if (otherOwner == Get_Owner())
        return;

    if (_hitCount >= _maxHitCount)
        return;

    if (_hitCooldowns[otherOwner.get()] > 0.f)
        return;

    Spawn_Effect_Once("Gemini_FireBall_Hit", _transformCom->Get_WorldPosition(), Vec3(2.f));

    if (auto character = dynamic_cast<Character*>(otherOwner.get()))
    {
        Apply_Skill_Hit(character, 10.f, _launchPower, _launchUp);
    }

    _hitCooldowns[otherOwner.get()] = (_hitInterval > 0.f) ? _hitInterval : 9999.f;
    ++_hitCount;
    Set_Destroy(true);
}

void Skill_FireBall::OnStayOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject_Projectile::OnStayOverlap(self, other);

}

void Skill_FireBall::OnEndOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject_Projectile::OnEndOverlap(self, other);

    if (!other)
        return;

    auto otherOwner = other->Get_Owner();
    if (!otherOwner)
        return;

    // 충돌이 끝난 대상의 쿨다운 적용 정리
    _hitCooldowns.erase(otherOwner.get());
}

Character* Skill_FireBall::Find_HitCharacter(Shared<Collider> other)
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

void Skill_FireBall::Process_Hit(Character* hitted, GameObject* targetKey)
{
    CHECK_NULL(hitted);
    CHECK_NULL(targetKey);

    if (!Apply_Skill_Hit(hitted, 10.f, _launchPower, _launchUp))
        return;

    Spawn_Effect_Once("Gemini_FireBall_Hit", _transformCom->Get_WorldPosition());

    Set_Destroy(true);
}

Shared<GameObject> Skill_FireBall::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_FireBall>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_FireBall");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_FireBall::Clone(void* arg)
{
    auto clone = make_shared<Skill_FireBall>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_FireBall");

        return nullptr;
    }

    return clone;
}

void Skill_FireBall::Free()
{
    SkillObject_Projectile::Free();
}
