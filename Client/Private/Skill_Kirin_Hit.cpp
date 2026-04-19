#include "pch.h"
#include "Skill_Kirin_Hit.h"
#include "GameObject_Factory.h"
#include "GameObject.h"
#include "Collider.h"
#include "Character.h"
#include "MyPlayer.h"
#include "EffectComponent.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_Kirin_Hit, Protocol::OBJECT_TYPE_KIRIN, "SkillSpawn");

Skill_Kirin_Hit::Skill_Kirin_Hit(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject(device, context)
{
}

Skill_Kirin_Hit::Skill_Kirin_Hit(const Skill_Kirin_Hit& rhs)
    : SkillObject(rhs)
{
}

HRESULT Skill_Kirin_Hit::Initialize_Prototype()
{
    _lifetime        = 15.f;
    _maxHitCount     = 6;
    _hitInterval     = 0.1f;
    _hitLaunchForce  = 0.f; 

    _colliderRadius  = 0.3f;
    _collisionPreset = Collision_Preset::Player_Attack;

    return SkillObject::Initialize_Prototype();
}

HRESULT Skill_Kirin_Hit::Initialize(void* arg)
{
    CHECK_FAILED(SkillObject::Initialize(arg), E_FAIL);

    EffectComponent::FPlayDesc playDesc{};
    playDesc.effectAssetName = "Kirin_Hit";
    playDesc.loopOverride = false;

    _collider->Set_IsActive(false);

    CHECK_FAILED(_effectCom->Play_Effect(playDesc), E_FAIL);


    return S_OK;
}

void Skill_Kirin_Hit::Update(float timeDelta)
{
    SkillObject::Update(timeDelta);

    if (Is_Destroy())
        return;


}

void Skill_Kirin_Hit::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnBeginOverlap(self, other);

    if (Is_Destroy())
        return;
    auto character = Find_HitCharacter(other);
    if (!character)
        return;

    auto otherOwner = other->Get_Owner();

    CHECK_NULL(otherOwner);

    Spawn_Effect_Once("Rasengan_Hit", _transformCom->Get_WorldPosition());

    Process_MultiHit(character, otherOwner.get());
}

void Skill_Kirin_Hit::OnStayOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnStayOverlap(self, other);

    if (Is_Destroy())
        return;
    auto character = Find_HitCharacter(other);
    if (!character)
        return;

    auto otherOwner = other->Get_Owner();

    CHECK_NULL(otherOwner);

    Process_MultiHit(character, otherOwner.get());
}

void Skill_Kirin_Hit::OnEndOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnEndOverlap(self, other);

    if (!other)
        return;

    auto otherOwner = other->Get_Owner();
    if (!otherOwner)
        return;

    _hitCooldowns.erase(otherOwner.get());
}

Character* Skill_Kirin_Hit::Find_HitCharacter(Shared<Collider> other)
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

void Skill_Kirin_Hit::Process_MultiHit(Character* hitted, GameObject* targetKey)
{
    CHECK_NULL(hitted);
    CHECK_NULL(targetKey);

    if (_hitCooldowns[targetKey] > 0.f)
        return;

    if (_hitCount >= _maxHitCount)
        return;

    if (!Apply_Skill_Hit(hitted, 10.f, _hitLaunchForce, 0.f))
        return;

    _hitCooldowns[targetKey] = _hitInterval;
    _hitCount++;
}

Shared<GameObject> Skill_Kirin_Hit::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_Kirin_Hit>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_Kirin_Hit");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_Kirin_Hit::Clone(void* arg)
{
    auto clone = make_shared<Skill_Kirin_Hit>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_Kirin_Hit");

        return nullptr;
    }

    return clone;
}

void Skill_Kirin_Hit::Free()
{
    SkillObject::Free();
}
