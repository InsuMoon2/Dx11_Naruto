#include "pch.h"
#include "Skill_Rasengan.h"
#include "GameObject_Factory.h"
#include "GameObject.h"
#include "Collider.h"
#include "Character.h"
#include "MyPlayer.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_Rasengan, Protocol::OBJECT_TYPE_SKILL_RASENGAN, "SkillSpawn");

Skill_Rasengan::Skill_Rasengan(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject(device, context)
{
}

Skill_Rasengan::Skill_Rasengan(const Skill_Rasengan& rhs)
    : SkillObject(rhs)
{
}

HRESULT Skill_Rasengan::Initialize_Prototype()
{
    _lifetime = 3.f;       
    _maxHitCount = 6;     
    _hitInterval = 0.1f;   
    _hitLaunchForce = 0.f; 

    _colliderRadius = 0.3f;
    _collisionPreset = Collision_Preset::Player_Attack;

    return SkillObject::Initialize_Prototype();
}

HRESULT Skill_Rasengan::Initialize(void* arg)
{
    CHECK_FAILED(SkillObject::Initialize(arg), E_FAIL);

    return S_OK;
}

void Skill_Rasengan::Update(float timeDelta)
{
    SkillObject::Update(timeDelta);

    if (Is_Destroy())
        return;



}

void Skill_Rasengan::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
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

void Skill_Rasengan::OnStayOverlap(Shared<Collider> self, Shared<Collider> other)
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

void Skill_Rasengan::OnEndOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnEndOverlap(self, other);

    if (!other)
        return;

    auto otherOwner = other->Get_Owner();
    if (!otherOwner)
        return;

    _hitCooldowns.erase(otherOwner.get());
}

Character* Skill_Rasengan::Find_HitCharacter(Shared<Collider> other)
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

void Skill_Rasengan::Process_MultiHit(Character* hitted, GameObject* targetKey)
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

Shared<GameObject> Skill_Rasengan::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_Rasengan>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_Rasengan");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_Rasengan::Clone(void* arg)
{
    auto clone = make_shared<Skill_Rasengan>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_Rasengan");

        return nullptr;
    }

    return clone;
}

void Skill_Rasengan::Free()
{
    SkillObject::Free();
}
