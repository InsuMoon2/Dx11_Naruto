#include "pch.h"
#include "Skill_Rasengan_Hit.h"
#include "GameObject_Factory.h"
#include "Collider.h"
#include "GameObject.h"
#include "Character.h"
#include "MyPlayer.h"
#include "EffectComponent.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_Rasengan_Hit, Protocol::OBJECT_TYPE_SKILL_RASENGAN_HIT, "SkillSpawn");

Skill_Rasengan_Hit::Skill_Rasengan_Hit(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject(device, context)
{
}

Skill_Rasengan_Hit::Skill_Rasengan_Hit(const Skill_Rasengan_Hit& rhs)
    : SkillObject(rhs)
    , _baseDamage(rhs._baseDamage)
    , _finalDamage(rhs._finalDamage)
    , _midHitLaunchUp(rhs._midHitLaunchUp)
    , _finalHitLaunchForce(rhs._finalHitLaunchForce)
    , _finalHitLaunchUp(rhs._finalHitLaunchUp)
    , _currentHitStep(rhs._currentHitStep)
{
}

HRESULT Skill_Rasengan_Hit::Initialize_Prototype()
{
    _lifetime        = 0.72f;   // 0.12초 간격으로 6번
    _maxHitCount     = 6;
    _hitInterval     = 0.12f;
    _hitLaunchForce  = 0.f;
    _colliderRadius  = 1.65f;
    _collisionPreset = Collision_Preset::Player_Attack;

    return SkillObject::Initialize_Prototype();
}

HRESULT Skill_Rasengan_Hit::Initialize(void* arg)
{
    CHECK_FAILED(SkillObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FSkillObjectDesc*>(arg);
    if (!desc)
        return E_FAIL;

    if (_transformCom)
        _transformCom->Set_WorldPosition(desc->spawnPosition);

    if (_collider)
        _collider->Set_IsActive(true);

    _currentHitStep = 0;
    _lastAppliedStepByTarget.clear();

    EffectComponent::FPlayDesc playDesc{};
    playDesc.effectAssetName = "Rasengan_Hit(2)";
    playDesc.localScale = Vec3(8.f, 8.f, 8.f);

    CHECK_FAILED(_effectCom->Play_Effect(playDesc), E_FAIL);

    return S_OK;
}

void Skill_Rasengan_Hit::Update(float timeDelta)
{
    SkillObject::Update(timeDelta);

    if (Is_Destroy())
        return;

    const int32 nextStep = min(_maxHitCount, static_cast<int32>(_elapsedTime / _hitInterval) + 1);
    _currentHitStep = nextStep;
}

void Skill_Rasengan_Hit::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
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

void Skill_Rasengan_Hit::OnStayOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnStayOverlap(self, other);

    if (Is_Destroy())
        return;

    auto character = Find_HitCharacter(other);
    CHECK_NULL(character);

    auto otherOwner = other->Get_Owner();
    CHECK_NULL(otherOwner);

    Process_MultiHit(character, otherOwner.get());
}

void Skill_Rasengan_Hit::OnEndOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnEndOverlap(self, other);
}

Character* Skill_Rasengan_Hit::Find_HitCharacter(Shared<Collider> other)
{
    if (!other || Is_Destroy())
        return nullptr;

    auto otherOwner = other->Get_Owner();
    if (!otherOwner)
        return nullptr;

    if (otherOwner == Get_Owner())
        return nullptr;

    return dynamic_cast<Character*>(otherOwner.get());
}

void Skill_Rasengan_Hit::Process_MultiHit(Character* hitted, GameObject* targetKey)
{
    CHECK_NULL(hitted);
    CHECK_NULL(targetKey);

    if (_currentHitStep <= 0 || _currentHitStep > _maxHitCount)
        return;

    if (_lastAppliedStepByTarget[targetKey] == _currentHitStep)
        return;

    const bool isFinalHit = (_currentHitStep == _maxHitCount);

    if (!isFinalHit)
    {
        if (!Apply_Skill_Hit(hitted, _baseDamage, 0.f, _midHitLaunchUp))
            return;
    }
    else
    {
        FDamageEvent event{};
        event.damage = _finalDamage;
        event.damageCauser = Get_Owner();
        event.launchPower = _finalHitLaunchForce;
        event.launchUp = _finalHitLaunchUp;

        hitted->TakeDamage(event);

        auto myPlayer = dynamic_pointer_cast<MyPlayer>(Get_Owner());
        if (myPlayer)
            myPlayer->Add_ComboHit();
    }

    _lastAppliedStepByTarget[targetKey] = _currentHitStep;
    ++_hitCount;
}

Shared<GameObject> Skill_Rasengan_Hit::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_Rasengan_Hit>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_Rasengan_Hit");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_Rasengan_Hit::Clone(void* arg)
{
    auto clone = make_shared<Skill_Rasengan_Hit>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_Rasengan_Hit");
        return nullptr;
    }

    return clone;
}

void Skill_Rasengan_Hit::Free()
{
    SkillObject::Free();
}
