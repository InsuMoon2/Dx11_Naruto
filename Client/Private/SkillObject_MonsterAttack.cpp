#include "pch.h"
#include "SkillObject_MonsterAttack.h"
#include "GameObject_Factory.h"
#include "Collider.h"
#include "Character.h"
#include "Client_Defines.h"

REGISTER_GAMEOBJECT_CATEGORY(SkillObject_MonsterAttack,
    Protocol::OBJECT_TYPE_SKILL_MONSTER_ATTACK, "MonsterAttackSpawn")

HRESULT SkillObject_MonsterAttack::Initialize_Prototype()
{
    _lifetime = 0.4f;
    _maxHitCount = 1;
    _hitInterval = 0.f;
    _colliderRadius = 0.8f;
    _collisionPreset = Collision_Preset::Monster_Attack;

    return SkillObject::Initialize_Prototype();
}

SkillObject_MonsterAttack::SkillObject_MonsterAttack(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject(device, context)
{
}

SkillObject_MonsterAttack::SkillObject_MonsterAttack(const SkillObject_MonsterAttack& rhs)
    : SkillObject(rhs)
    , _damage(rhs._damage)
{
}

HRESULT SkillObject_MonsterAttack::Initialize(void* arg)
{
    CHECK_FAILED(SkillObject::Initialize(arg), E_FAIL);

    if (arg)
    {
        auto* desc = static_cast<FSkillObjectDesc*>(arg);
        _colliderRadius = desc->colliderRadius;
    }

    return S_OK;
}

void SkillObject_MonsterAttack::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnBeginOverlap(self, other);

    if (Is_Destroy())
        return;

    auto character = Find_HitCharacter(other);
    if (!character)
        return;

    auto otherOwner = other->Get_Owner();
    CHECK_NULL(otherOwner);

    Process_Hit(character, otherOwner.get());
}

void SkillObject_MonsterAttack::OnStayOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnStayOverlap(self, other);
}

void SkillObject_MonsterAttack::OnEndOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnEndOverlap(self, other);

    if (!other)
        return;

    auto otherOwner = other->Get_Owner();
    if (otherOwner)
        _hitCooldowns.erase(otherOwner.get());
}

Character* SkillObject_MonsterAttack::Find_HitCharacter(Shared<Collider> other)
{
    if (!other || Is_Destroy())
        return nullptr;

    auto otherOwner = other->Get_Owner();
    if (!otherOwner)
        return nullptr;

    // 발사한 몬스터(오너)는 자기 자신을 치지 않는다
    if (otherOwner == Get_Owner())
        return nullptr;

    return dynamic_cast<Character*>(otherOwner.get());
}

void SkillObject_MonsterAttack::Process_Hit(Character* hitted, GameObject* targetKey)
{
    CHECK_NULL(hitted);
    CHECK_NULL(targetKey);

    if (_hitCount >= _maxHitCount)
        return;

    if (_hitCooldowns[targetKey] > 0.f)
        return;

    if (!Apply_Skill_Hit(hitted, _damage, 0.f, 0.f))
        return;

    _hitCooldowns[targetKey] = _hitInterval;
    _hitCount++;

    Set_Destroy(true);
}

Shared<GameObject> SkillObject_MonsterAttack::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<SkillObject_MonsterAttack>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : SkillObject_MonsterAttack");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> SkillObject_MonsterAttack::Clone(void* arg)
{
    auto clone = make_shared<SkillObject_MonsterAttack>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : SkillObject_MonsterAttack");
        return nullptr;
    }

    return clone;
}

void SkillObject_MonsterAttack::Free()
{
    SkillObject::Free();
}
