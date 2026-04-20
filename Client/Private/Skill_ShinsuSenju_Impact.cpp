#include "pch.h"
#include "Skill_ShinsuSenju_Impact.h"

#include "GameObject_Factory.h"
#include "GameObject.h"
#include "Collider.h"
#include "Character.h"
#include "EffectComponent.h"
#include "MyPlayer.h"
#include "Client_Defines.h"
#include "MeshDebrisObject.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_ShinsuSenju_Impact,
                             Protocol::OBJECT_TYPE_SHINSUSENJU_IMPACT, "SkillSpawn")

Skill_ShinsuSenju_Impact::Skill_ShinsuSenju_Impact(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject(device, context)
{
}

Skill_ShinsuSenju_Impact::Skill_ShinsuSenju_Impact(const Skill_ShinsuSenju_Impact& rhs)
    : SkillObject(rhs)
    , _damage(rhs._damage)
    , _launchUp(rhs._launchUp)
{
}

HRESULT Skill_ShinsuSenju_Impact::Initialize_Prototype()
{
    _lifetime = 0.12f;
    _maxHitCount = 32;
    _hitInterval = 0.f;
    _hitLaunchForce = 5.5f;
    _colliderRadius = 3.5f;
    _collisionPreset = Collision_Preset::Player_Attack;
    _effectAssetName = "ShinsuSenju_Impact";

    return SkillObject::Initialize_Prototype();
}

HRESULT Skill_ShinsuSenju_Impact::Initialize(void* arg)
{
    CHECK_FAILED(SkillObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FImpactDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);

    _damage = desc->damage;
    _hitLaunchForce = desc->launchForce;
    _launchUp = desc->launchUp;
    _lifetime = desc->lingerTime;

    _hitCount = 0;
    _hitCooldowns.clear();

    if (_transformCom)
    {
        _transformCom->Set_WorldPosition(desc->spawnPosition);
        _transformCom->Set_LocalRotation(
            desc->spawnRotation.x,
            desc->spawnRotation.y,
            desc->spawnRotation.z);
        _transformCom->Set_LocalScale(desc->scale);
    }

    if (_collider)
    {
        _collider->Set_IsActive(true);
        _collider->Set_CollisionPreset(_collisionPreset);
    }

    Spawn_Particle("SmallRock", 10, desc->spawnPosition);

    CHECK_FAILED(Play_ImpactEffect(*desc), E_FAIL);

    return S_OK;
}

void Skill_ShinsuSenju_Impact::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnBeginOverlap(self, other);

    if (Is_Destroy())
        return;

    Character* character = Find_HitCharacter(other);
    if (!character)
        return;

    Shared<GameObject> otherOwner = other->Get_Owner();
    CHECK_NULL(otherOwner);

    Process_Hit(character, otherOwner.get());
}

void Skill_ShinsuSenju_Impact::OnStayOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnStayOverlap(self, other);

    if (Is_Destroy())
        return;

    Character* character = Find_HitCharacter(other);
    if (!character)
        return;

    Shared<GameObject> otherOwner = other->Get_Owner();
    CHECK_NULL(otherOwner);

    Process_Hit(character, otherOwner.get());
}

void Skill_ShinsuSenju_Impact::OnEndOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnEndOverlap(self, other);
}

Character* Skill_ShinsuSenju_Impact::Find_HitCharacter(Shared<Collider> other)
{
    if (!other || Is_Destroy())
        return nullptr;

    Shared<GameObject> otherOwner = other->Get_Owner();
    if (!otherOwner)
        return nullptr;

    if (otherOwner == Get_Owner())
        return nullptr;

    return dynamic_cast<Character*>(otherOwner.get());
}

void Skill_ShinsuSenju_Impact::Process_Hit(Character* hitted, GameObject* targetKey)
{
    CHECK_NULL(hitted);
    CHECK_NULL(targetKey);

    if (_hitCount >= _maxHitCount)
        return;

    if (_hitCooldowns[targetKey] > 0.f)
        return;

    Shared<GameObject> owner = Get_Owner();
    CHECK_NULL(owner);

    Vec3 damageDir = hitted->Get_Transform()->Get_WorldPosition() - _transformCom->Get_WorldPosition();
    damageDir.y = 0.f;
    damageDir = Utils::Safe_Normalize(damageDir, Vec3::Forward);

    FDamageEvent damageEvent{};
    damageEvent.damage = _damage;
    damageEvent.damageCauser = owner;
    damageEvent.hasCustomDir = true;
    damageEvent.damageDir = damageDir;
    damageEvent.launchPower = _hitLaunchForce;
    damageEvent.launchUp = _launchUp;

    hitted->TakeDamage(damageEvent);

    if (auto myPlayer = dynamic_pointer_cast<MyPlayer>(owner))
        myPlayer->Add_ComboHit();

    _hitCooldowns[targetKey] = FLT_MAX;
    ++_hitCount;
}

HRESULT Skill_ShinsuSenju_Impact::Play_ImpactEffect(const FImpactDesc& desc)
{
    CHECK_NULL(_effectCom, E_FAIL);

    if (_effectAssetName.empty())
        return S_OK;

    EffectComponent::FPlayDesc playDesc{};
    playDesc.effectAssetName = _effectAssetName;
    playDesc.localScale = desc.effectScale;
    playDesc.loopOverride = false;

    return _effectCom->Play_Effect(playDesc);
}

void Skill_ShinsuSenju_Impact::Spawn_Particle(const string& assetName, int32 spawnIndex, const Vec3 spawnPos)
{
    const int32 safeCount = (spawnIndex <= 0) ? 1 : spawnIndex;

    for (int32 i = 0; i < safeCount; ++i)
    {
        MeshDebrisObject::FMeshDebrisDesc debrisDesc{};

        debrisDesc.effectAssetName = assetName;
        debrisDesc.position = spawnPos;

        debrisDesc.spawnRotation = Vec3(
            Utils::RandomRange(0.f, 360.f),
            Utils::RandomRange(0.f, 360.f),
            Utils::RandomRange(0.f, 360.f));

        debrisDesc.spawnScale = Vec3(1.f, 1.f, 1.f);

        const float randomScale = Utils::RandomRange(0.10f, 0.18f);
        debrisDesc.effectLocalScale = Vec3(randomScale, randomScale, randomScale);

        debrisDesc.initialVelocity = Vec3(
            Utils::RandomRange(-3.5f, 3.5f),
            Utils::RandomRange(7.f, 10.f),
            Utils::RandomRange(-3.5f, 3.5f));

        debrisDesc.gravity = -24.f;

        debrisDesc.angularVelocityDeg = Vec3(
            Utils::RandomRange(-360.f, 360.f),
            Utils::RandomRange(-360.f, 360.f),
            Utils::RandomRange(-360.f, 360.f));

        debrisDesc.lifetime = Utils::RandomRange(0.8f, 1.2f);
        debrisDesc.groundY = spawnPos.y;

        debrisDesc.destroyOnGroundHit = false;
        debrisDesc.stopOnGroundHit = true;

        GAME->Clone_And_Add_GameObject(
            ETOI(ELevelType::Static),
            Protocol::OBJECT_TYPE_MESH_DEBRIS,
            GAME->Current_Level(),
            TEXT("Layer_Effect"),
            &debrisDesc);
    }
}

Shared<GameObject> Skill_ShinsuSenju_Impact::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_ShinsuSenju_Impact>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_ShinsuSenju_Impact");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_ShinsuSenju_Impact::Clone(void* arg)
{
    auto clone = make_shared<Skill_ShinsuSenju_Impact>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_ShinsuSenju_Impact");
        return nullptr;
    }

    return clone;
}

void Skill_ShinsuSenju_Impact::Free()
{
    SkillObject::Free();
}
