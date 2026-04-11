#include "pch.h"
#include "Skill_RasenShuriken.h"
#include "GameObject_Factory.h"
#include "Collider.h"
#include "GameObject.h"
#include "Character.h"
#include "Bounding_Sphere.h"
#include "MyPlayer.h"
#include "EffectComponent.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_RasenShuriken, Protocol::OBJECT_TYPE_SKILL_RASENSHURIKEN, "SkillSpawn");

Skill_RasenShuriken::Skill_RasenShuriken(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject_Projectile(device, context)
{
}

Skill_RasenShuriken::Skill_RasenShuriken(const Skill_RasenShuriken& rhs)
    : SkillObject_Projectile(rhs)
    , _isActivatedByHit(rhs._isActivatedByHit)
    , _activationPosition(rhs._activationPosition)
    , _baseScale(rhs._baseScale)
    , _maxScale(rhs._maxScale)
    , _scaleGrowSpeed(rhs._scaleGrowSpeed)
    , _midHitLaunchUp(rhs._midHitLaunchUp)
    , _finalBlastRadius(rhs._finalBlastRadius)
    , _finalBlastLaunchPower(rhs._finalBlastLaunchPower)
    , _finalBlastLaunchUp(rhs._finalBlastLaunchUp)
    , _isFinalBlast(rhs._isFinalBlast)
{
}

HRESULT Skill_RasenShuriken::Initialize_Prototype()
{
    _speed = 28.f;
    _maxDistance = 35.f;
    _lifetime = 5.5f;
    _maxHitCount = 5;
    _hitInterval = 0.12f;
    _hitLaunchForce = 2.5f;
    _colliderRadius = 1.35f;

    _collisionPreset = Collision_Preset::Player_Attack;

    _isMoving = false;

    return SkillObject_Projectile::Initialize_Prototype();
}

HRESULT Skill_RasenShuriken::Initialize(void* arg)
{
    CHECK_FAILED(SkillObject_Projectile::Initialize(arg), E_FAIL);

    _isMoving = false;
    _isActivatedByHit = false;
    _activationPosition = Vec3::Zero;

    if (_transformCom)
        _transformCom->Set_LocalScale(_baseScale);

    EffectComponent::FPlayDesc playDesc{};
    playDesc.effectAssetName = "RasenganShuriken";

    CHECK_FAILED(_effectCom->Play_Effect(playDesc), E_FAIL);

    return S_OK;
}

void Skill_RasenShuriken::Update(float timeDelta)
{
    SkillObject_Projectile::Update(timeDelta);

    if (Is_Destroy())
        return;

    // 첫 충돌 이후에는 위치 고정
    if (_isActivatedByHit && _transformCom)
    {
        _transformCom->Set_WorldPosition(_activationPosition);

        Vec3 currentScale = _transformCom->Get_LocalScale();
        float currentRadius = _maxScale;
        float nextScale = min(_maxScale, currentScale.x + (_scaleGrowSpeed * timeDelta));

        Shared<Bounding_Sphere> sphere = static_pointer_cast<Bounding_Sphere>(_collider->Get_Bounding());
        if (sphere)
            sphere->Get_OriginSphere().Radius = nextScale; 

    }
    
}

void Skill_RasenShuriken::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject_Projectile::OnBeginOverlap(self, other);

    if (Is_Destroy())
        return;

    auto character = Find_HitCharacter(other);
    if (!character)
        return;

    auto otherOwner = other->Get_Owner();
    CHECK_NULL(otherOwner);

    if (!_isActivatedByHit)
    {
        _isActivatedByHit = true;
        _isMoving = false;

        if (_transformCom)
            _activationPosition = _transformCom->Get_WorldPosition();
    }

    Process_MultiHit(character, otherOwner.get());
}

void Skill_RasenShuriken::OnStayOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject_Projectile::OnStayOverlap(self, other);

    if (!_isActivatedByHit || Is_Destroy())
        return;

    auto character = Find_HitCharacter(other);
    CHECK_NULL(character);

    auto otherOwner = other->Get_Owner();
    CHECK_NULL(otherOwner);

    Process_MultiHit(character, otherOwner.get());

    if (_isFinalBlast && !_hitCooldowns.empty())
    {
        Set_Destroy(true);
    }
}

void Skill_RasenShuriken::OnEndOverlap(Shared<Collider> self, Shared<Collider> other)
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

void Skill_RasenShuriken::Process_MultiHit(Character* hitted, GameObject* targetKey)
{
    CHECK_NULL(hitted);
    CHECK_NULL(targetKey);

    if (_hitCooldowns[targetKey] > 0.f)
        return;

    if (_hitCount >= _maxHitCount -1)
    {
        _isFinalBlast = true;
        _hitCooldowns.clear();

        if (_transformCom)
            _transformCom->Set_LocalScale(_finalBlastRadius);

        Trigger_FinalHit(hitted, targetKey);

        return;
    }

    if (!Apply_Skill_Hit(hitted, 10.f, 0.f, _midHitLaunchUp))
        return;

    _hitCooldowns[targetKey] = _hitInterval;
    _hitCount++;
}

void Skill_RasenShuriken::Trigger_FinalHit(Character* character, GameObject* targetKey)
{
    CHECK_NULL(character);
    CHECK_NULL(targetKey);

    if (_hitCooldowns[targetKey] > 0.f)
        return;

    Vec3 dir = character->Get_Transform()->Get_WorldPosition() - _activationPosition;
    dir.y = 0.f;
    dir = Utils::Safe_Normalize(dir);

    FDamageEvent event{};
    event.damage = 20.f;
    event.damageCauser = Get_Owner();
    event.hasCustomDir = true;
    event.damageDir = dir;
    event.launchPower = _finalBlastLaunchPower;
    event.launchUp = _finalBlastLaunchUp;

    character->TakeDamage(event);

    auto myPlayer = dynamic_pointer_cast<MyPlayer>(Get_Owner());
    if (myPlayer)
        myPlayer->Add_ComboHit();

    _hitCooldowns[targetKey] = FLT_MAX;
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
