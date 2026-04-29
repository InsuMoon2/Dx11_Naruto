#include "pch.h"
#include "Skill_FireBall.h"
#include "GameObject_Factory.h"
#include "Collider.h"
#include "GameObject.h"
#include "Character.h"
#include "Bounding_Sphere.h"
#include "MyPlayer.h"
#include "EffectComponent.h"
#include "Utils.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_FireBall, Protocol::OBJECT_TYPE_SKILL_FIREBALL, "SkillSpawn");

Skill_FireBall::Skill_FireBall(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject_Projectile(device, context)
{
}

Skill_FireBall::Skill_FireBall(const Skill_FireBall& rhs)
    : SkillObject_Projectile(rhs)
    , _launchPower(rhs._launchPower)
    , _launchUp(rhs._launchUp)
    , _hasExploded(rhs._hasExploded)
{
}

HRESULT Skill_FireBall::Initialize_Prototype()
{
    _speed = 28.f;
    _maxDistance = 8.f;  // RasenShuriken(7.f)처럼 maxDistance로 사거리 제어 → 발사 8유닛 후 폭발
    _lifetime = 0.6f;    // maxDistance가 안 던져질 상황의 fallback 안전망
    _maxHitCount = 32;
    _hitLaunchForce = _launchPower;
    _damage = _explosionDamage;
    _colliderRadius = 2.35f;

    _collisionPreset = Collision_Preset::Player_Attack;

    _isMoving = true;

    return SkillObject_Projectile::Initialize_Prototype();
}

HRESULT Skill_FireBall::Initialize(void* arg)
{
    CHECK_FAILED(SkillObject_Projectile::Initialize(arg), E_FAIL);

    _isMoving = true;
    _hasExploded = false;

    GAME->Play_LoopSound(L"Fireball_Loop.wav", ESoundChannel::Effect, 0.3f, false);

    EffectComponent::FPlayDesc playDesc{};
    playDesc.effectAssetName = "Last_FireBall_Loop";

    CHECK_FAILED(_effectCom->Play_Effect(playDesc), E_FAIL);

    return S_OK;
}

void Skill_FireBall::Update(float timeDelta)
{
    SkillObject_Projectile::Update(timeDelta);

    if (Is_Destroy())
    {
        Explode_FireBall();
        return;
    }
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

    Explode_FireBall();

    _hitCooldowns[otherOwner.get()] = (_hitInterval > 0.f) ? _hitInterval : 9999.f;
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

void Skill_FireBall::Explode_FireBall()
{
    if (_hasExploded)
        return;

    _hasExploded = true;
    const Vec3 explosionCenter = _transformCom ? _transformCom->Get_WorldPosition() : Vec3::Zero;
    GAME->Stop_Sound(L"Fireball_Loop.wav");
    GAME->Play_Sound(L"Fireball_Hit.wav", ESoundChannel::Effect, 0.3f);
    Spawn_Effect_Once("Last_FireBall_Hit", explosionCenter, Vec3(2.f));
    Apply_ExplosionAreaDamage(explosionCenter);
}

void Skill_FireBall::Process_Hit(Character* hitted, GameObject* targetKey)
{
    CHECK_NULL(hitted);
    CHECK_NULL(targetKey);

    if (!Apply_Skill_Hit(hitted, _explosionDamage, _launchPower, _launchUp))
        return;

    Explode_FireBall();
    

    Set_Destroy(true);
}

void Skill_FireBall::Apply_ExplosionAreaDamage(const Vec3& explosionCenter)
{
    const float explosionRadiusSq = _explosionRadius * _explosionRadius;
    Shared<GameObject> owner = Get_Owner();

    for (const auto& obj : GAME->Get_GameObjects(Get_LevelIndex()))
    {
        if (!obj || obj == owner || obj->Is_Destroy())
            continue;

        Character* character = dynamic_cast<Character*>(obj.get());
        if (!character)
            continue;

        auto targetTransform = obj->Get_Transform();
        if (!targetTransform)
            continue;

        const Vec3 targetPos = targetTransform->Get_WorldPosition();
        Vec3 damageDir = targetPos - explosionCenter;
        damageDir.y = 0.f;

        if (damageDir.LengthSquared() > explosionRadiusSq)
            continue;

        damageDir = Utils::Safe_Normalize(damageDir, _transformCom ? _transformCom->Get_WorldForward() : Vec3::Forward);

        FDamageEvent damageEvent{};
        damageEvent.damage = _explosionDamage;
        damageEvent.damageCauser = owner;
        damageEvent.hasCustomDir = true;
        damageEvent.damageDir = damageDir;
        damageEvent.launchPower = _launchPower;
        damageEvent.launchUp = _launchUp;

        character->TakeDamage(damageEvent);

        if (auto myPlayer = dynamic_pointer_cast<MyPlayer>(owner))
            myPlayer->Add_ComboHit();

        _hitCooldowns[obj.get()] = FLT_MAX;
        ++_hitCount;
    }
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
    GAME->Stop_Sound(L"Fireball_Loop.wav");
    SkillObject_Projectile::Free();
}
