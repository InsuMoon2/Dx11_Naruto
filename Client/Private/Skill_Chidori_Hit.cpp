#include "pch.h"
#include "Skill_Chidori_Hit.h"
#include "GameObject_Factory.h"
#include "Collider.h"
#include "GameObject.h"
#include "Character.h"
#include "Bounding_Sphere.h"
#include "MyPlayer.h"
#include "EffectComponent.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_Chidori_Hit, Protocol::OBJECT_TYPE_SKILL_CHIDORI_HIT, "SkillSpawn");

Skill_Chidori_Hit::Skill_Chidori_Hit(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject(device, context)
{
}

Skill_Chidori_Hit::Skill_Chidori_Hit(const Skill_Chidori_Hit& rhs)
    : SkillObject(rhs)
    , _baseScale(rhs._baseScale)
    , _midHitLaunchUp(rhs._midHitLaunchUp)
    , _finalBlastRadius(rhs._finalBlastRadius)
    , _finalBlastLaunchPower(rhs._finalBlastLaunchPower)
    , _finalBlastLaunchUp(rhs._finalBlastLaunchUp)
    , _isFinalBlast(rhs._isFinalBlast)
{
}

HRESULT Skill_Chidori_Hit::Initialize_Prototype()
{
    _maxHitCount = 5;
    _hitInterval = 0.12f;
    _hitLaunchForce = 2.5f;
    _colliderRadius = 1.35f;

    _collisionPreset = Collision_Preset::Player_Attack;

    return SkillObject::Initialize_Prototype();
}

HRESULT Skill_Chidori_Hit::Initialize(void* arg)
{
    CHECK_FAILED(SkillObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FHitDesc*>(arg);
    if (!desc) return E_FAIL;

     if (desc->damageCauser)
        Set_Owner(desc->damageCauser);

    _isFinalBlast = false;

    if (_transformCom)
    {
        _transformCom->Set_WorldPosition(desc->spawnPosition);
        _transformCom->Set_LocalScale(_baseScale);
    }

    if (_collider)
        _collider->Set_IsActive(true);

    EffectComponent::FPlayDesc playDesc{};
    playDesc.effectAssetName = "Chidori_Point";

    CHECK_FAILED(_effectCom->Play_Effect(playDesc), E_FAIL);

    return S_OK;
}

void Skill_Chidori_Hit::Update(float timeDelta)
{
    SkillObject::Update(timeDelta);

    if (Is_Destroy())
        return;

}

void Skill_Chidori_Hit::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnBeginOverlap(self, other);

    if (Is_Destroy())
        return;

    auto character = Find_HitCharacter(other);
    if (!character)
        return;

    auto otherOwner = other->Get_Owner();
    CHECK_NULL(otherOwner);

    Process_MultiHit(character, otherOwner.get());
}

void Skill_Chidori_Hit::OnStayOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnStayOverlap(self, other);

    if (Is_Destroy())
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

void Skill_Chidori_Hit::OnEndOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnEndOverlap(self, other);

    if (!other)
        return;

    auto otherOwner = other->Get_Owner();
    if (!otherOwner)
        return;

    // 충돌이 끝난 대상의 쿨다운 적용 정리
    _hitCooldowns.erase(otherOwner.get());
}

Character* Skill_Chidori_Hit::Find_HitCharacter(Shared<Collider> other)
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

void Skill_Chidori_Hit::Process_MultiHit(Character* hitted, GameObject* targetKey)
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
        {
            _transformCom->Set_LocalScale(_finalBlastRadius);
            
            Shared<Bounding_Sphere> sphere = static_pointer_cast<Bounding_Sphere>(_collider->Get_Bounding());
            if (sphere)
                sphere->Get_OriginSphere().Radius = _finalBlastRadius;
        }

        Trigger_FinalHit(hitted, targetKey);

        return;
    }

    if (!Apply_Skill_Hit(hitted, 10.f, 0.f, _midHitLaunchUp))
        return;

    _hitCooldowns[targetKey] = _hitInterval;
    _hitCount++;
}

void Skill_Chidori_Hit::Trigger_FinalHit(Character* character, GameObject* targetKey)
{
    CHECK_NULL(character);
    CHECK_NULL(targetKey);

    if (_hitCooldowns[targetKey] > 0.f)
        return;

    Vec3 dir = character->Get_Transform()->Get_WorldPosition() -  _transformCom->Get_WorldPosition();
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

    // 두번 히트 안되게
    _hitCooldowns[targetKey] = FLT_MAX;
}

Shared<GameObject> Skill_Chidori_Hit::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_Chidori_Hit>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_Chidori_Hit");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_Chidori_Hit::Clone(void* arg)
{
    auto clone = make_shared<Skill_Chidori_Hit>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_Chidori_Hit");

        return nullptr;
    }

    return clone;
}

void Skill_Chidori_Hit::Free()
{
    SkillObject::Free();
}
