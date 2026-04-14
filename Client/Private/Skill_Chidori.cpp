#include "pch.h"
#include "Skill_Chidori.h"
#include "GameObject_Factory.h"
#include "GameObject.h"
#include "Collider.h"
#include "Character.h"
#include "MyPlayer.h"
#include "EffectComponent.h"
#include "Character.h"
#include "Model.h"
#include "Skill_Chidori_Hit.h"
#include "StretchingMeshEffect.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_Chidori, Protocol::OBJECT_TYPE_SKILL_CHIDORI, "SkillSpawn");

Skill_Chidori::Skill_Chidori(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject(device, context)
{
}

Skill_Chidori::Skill_Chidori(const Skill_Chidori& rhs)
    : SkillObject(rhs)
{
}

HRESULT Skill_Chidori::Initialize_Prototype()
{
    _lifetime        = 15.f;
    _maxHitCount     = 1;
    _hitInterval     = 0.05f;   
    _hitLaunchForce  = 0.f; 

    _colliderRadius  = 0.3f;
    _collisionPreset = Collision_Preset::Player_Attack;

    return SkillObject::Initialize_Prototype();
}

HRESULT Skill_Chidori::Initialize(void* arg)
{
    CHECK_FAILED(SkillObject::Initialize(arg), E_FAIL);

    return S_OK;
}

void Skill_Chidori::Update(float timeDelta)
{
    SkillObject::Update(timeDelta);

    if (Is_Destroy())
        return;


}

void Skill_Chidori::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnBeginOverlap(self, other);

    if (Is_Destroy())
        return;
    auto character = Find_HitCharacter(other);
    if (!character)
        return;

    auto otherOwner = other->Get_Owner();

    CHECK_NULL(otherOwner);

    if (_hitCount >= _maxHitCount)
        return;

    Vec3 hitDir = _transformCom->Get_WorldForward();
    hitDir = Utils::Safe_Normalize(hitDir);

    FDamageEvent event{};
    event.damage = 10.f;
    event.damageCauser = Get_Owner();
    event.hasCustomDir = true;
    event.damageDir = hitDir;
    event.launchPower = 2.5f;
    event.launchUp = 0.f;

    character->TakeDamage(event);

    auto myPlayer = dynamic_pointer_cast<MyPlayer>(Get_Owner());
    if (myPlayer)
        myPlayer->Add_ComboHit();

    _hitCount++;

    if (_collider)
        _collider->Set_IsActive(false);

    _lifetime = min(_lifetime, _elapsedTime + 0.08f);

    // 히트되면 카메라 연출

}

void Skill_Chidori::OnEndOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnEndOverlap(self, other);

    if (!other)
        return;

    auto otherOwner = other->Get_Owner();
    if (!otherOwner)
        return;

    _hitCooldowns.erase(otherOwner.get());
}

Character* Skill_Chidori::Find_HitCharacter(Shared<Collider> other)
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

void Skill_Chidori::Process_MultiHit(Character* hitted, GameObject* targetKey)
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


Shared<GameObject> Skill_Chidori::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_Chidori>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_Chidori");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_Chidori::Clone(void* arg)
{
    auto clone = make_shared<Skill_Chidori>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_Chidori");

        return nullptr;
    }

    return clone;
}

void Skill_Chidori::Free()
{
    SkillObject::Free();
}
