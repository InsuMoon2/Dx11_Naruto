#include "pch.h"
#include "Skill_Kamui.h"
#include "GameObject_Factory.h"
#include "GameObject.h"
#include "Collider.h"
#include "Character.h"
#include "MyPlayer.h"
#include "EffectComponent.h"
#include "Transform.h"
#include "Utils.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_Kamui, Protocol::OBJECT_TYPE_SKILL_KAMUI, "SkillSpawn");

Skill_Kamui::Skill_Kamui(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject(device, context)
{
}

Skill_Kamui::Skill_Kamui(const Skill_Kamui& rhs)
    : SkillObject(rhs)
    , _baseDamage(rhs._baseDamage)
    , _finalDamage(rhs._finalDamage)
    , _midHitLaunchForce(rhs._midHitLaunchForce)
    , _midHitLaunchUp(rhs._midHitLaunchUp)
    , _finalHitLaunchForce(rhs._finalHitLaunchForce)
    , _finalHitLaunchUp(rhs._finalHitLaunchUp)
    , _currentHitStep(rhs._currentHitStep)
{
}

HRESULT Skill_Kamui::Initialize_Prototype()
{
    _lifetime = 1.0f; 
    _maxHitCount = 10;
    _hitInterval = 0.10f;
    _hitLaunchForce = 0.f;

    _colliderRadius = 1.8f;
    _collisionPreset = Collision_Preset::Player_Attack;

    return SkillObject::Initialize_Prototype();
}

HRESULT Skill_Kamui::Initialize(void* arg)
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
    playDesc.effectAssetName = "Kamui";
    playDesc.loopOverride = false;

    if (_effectCom)
        CHECK_FAILED(_effectCom->Play_Effect(playDesc), E_FAIL);

    return S_OK;
}

void Skill_Kamui::Update(float timeDelta)
{
    SkillObject::Update(timeDelta);

    if (Is_Destroy())
        return;

    const int32 nextStep = min(_maxHitCount, static_cast<int32>(_elapsedTime / _hitInterval) + 1);
    _currentHitStep = nextStep;
}

void Skill_Kamui::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnBeginOverlap(self, other);

    if (Is_Destroy())
        return;

    auto* character = Find_HitCharacter(other);
    if (!character)
        return;

    auto otherOwner = other->Get_Owner();
    CHECK_NULL(otherOwner);

    Process_MultiHit(character, otherOwner.get());
}

void Skill_Kamui::OnStayOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnStayOverlap(self, other);

    if (Is_Destroy())
        return;

    auto* character = Find_HitCharacter(other);
    if (!character)
        return;

    auto otherOwner = other->Get_Owner();
    CHECK_NULL(otherOwner);

    Process_MultiHit(character, otherOwner.get());
}

void Skill_Kamui::OnEndOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnEndOverlap(self, other);

    if (!other)
        return;

    auto otherOwner = other->Get_Owner();
    if (!otherOwner)
        return;

    _lastAppliedStepByTarget.erase(otherOwner.get());
}

Character* Skill_Kamui::Find_HitCharacter(Shared<Collider> other)
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

void Skill_Kamui::Process_MultiHit(Character* hitted, GameObject* targetKey)
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
        if (!Apply_Skill_Hit(hitted, _baseDamage, _midHitLaunchForce, _midHitLaunchUp))
            return;
    }
    else
    {
        FDamageEvent damageEvent{};
        damageEvent.damage = _finalDamage;
        damageEvent.damageCauser = Get_Owner();
        damageEvent.launchPower = _finalHitLaunchForce;
        damageEvent.launchUp = _finalHitLaunchUp;
        damageEvent.forceHitRestart = true;
        damageEvent.hitReactionType = EHitReactionType::Launch;

        Vec3 damageDir = Vec3::Zero;
        if (_transformCom)
        {
            damageDir = _transformCom->Get_WorldForward();
            damageDir.y = 0.f;
            damageDir = Utils::Safe_Normalize(damageDir, Vec3::Zero);
        }

        if (damageDir.LengthSquared() > FLT_EPSILON)
        {
            damageEvent.damageDir = damageDir;
            damageEvent.hasCustomDir = true;
        }

        hitted->TakeDamage(damageEvent);

        auto myPlayer = dynamic_pointer_cast<MyPlayer>(Get_Owner());
        if (myPlayer)
            myPlayer->Add_ComboHit();
    }

    _lastAppliedStepByTarget[targetKey] = _currentHitStep;
    ++_hitCount;
}

Shared<GameObject> Skill_Kamui::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_Kamui>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_Kamui");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_Kamui::Clone(void* arg)
{
    auto clone = make_shared<Skill_Kamui>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_Kamui");
        return nullptr;
    }

    return clone;
}

void Skill_Kamui::Free()
{
    SkillObject::Free();
}
