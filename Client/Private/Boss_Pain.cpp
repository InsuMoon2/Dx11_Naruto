#include "pch.h"
#include "Boss_Pain.h"

#include "CombatStat.h"
#include "MovementComponent.h"
#include "AIController.h"
#include "BehaviorTree.h"
#include "Shader.h"
#include "Model.h"
#include "AnimationStateComponent.h"
#include "GameObject_Factory.h"
#include "Bounding_OBB.h"
#include "Collider.h"

REGISTER_GAMEOBJECT(Boss_Pain, Protocol::OBJECT_TYPE_BOSS_PAIN)

NS_BEGIN(Client)

Boss_Pain::Boss_Pain(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : EnemyCharacter(device, context)
{
    _hitEffectHeightOffset = 1.5f;
}

Boss_Pain::Boss_Pain(const Boss_Pain& rhs)
    : EnemyCharacter(rhs)
    , _maxHp(rhs._maxHp)
    , _attack(rhs._attack)
    , _maxWalkSpeed(rhs._maxWalkSpeed)
    , _maxSprintSpeed(rhs._maxSprintSpeed)
    , _bodyColliderCenter(rhs._bodyColliderCenter)
    , _bodyColliderExtents(rhs._bodyColliderExtents)
    , _modelComponentID(rhs._modelComponentID)
{
}

HRESULT Boss_Pain::Initialize_Prototype()
{
    CHECK_FAILED(EnemyCharacter::Initialize_Prototype(), E_FAIL);

    return S_OK;
}

HRESULT Boss_Pain::Initialize(void* arg)
{
    CHECK_FAILED(EnemyCharacter::Initialize(arg), E_FAIL);

    return S_OK;
}

void Boss_Pain::BeginPlay()
{
    EnemyCharacter::BeginPlay();

    GAME->Get_DelegateHub().OnBossObjectSpawned.Broadcast(GetSharedPtr<Boss_Pain>());
}

json Boss_Pain::To_Json() const
{
    json j = Character::To_Json();

    j["max_hp"] = _maxHp;
    j["attack"] = _attack;
    j["max_walk_speed"] = _maxWalkSpeed;
    j["max_sprint_speed"] = _maxSprintSpeed;
    j["body_collider_center"] = { _bodyColliderCenter.x, _bodyColliderCenter.y, _bodyColliderCenter.z };
    j["body_collider_extents"] = { _bodyColliderExtents.x, _bodyColliderExtents.y, _bodyColliderExtents.z };
    j["network_driven"] = _networkDriven;
    j["hit_effect_asset_name"] = _hitEffectAssetName;
    j["idle_state_name"] = _idleStateName;

    return j;
}

void Boss_Pain::From_Json(const json& data)
{
    Character::From_Json(data);

    if (data.contains("max_hp"))
        _maxHp = data["max_hp"].get<float>();

    if (data.contains("attack"))
        _attack = data["attack"].get<float>();

    if (data.contains("max_walk_speed"))
        _maxWalkSpeed = data["max_walk_speed"].get<float>();

    if (data.contains("max_sprint_speed"))
        _maxSprintSpeed = data["max_sprint_speed"].get<float>();

    if (data.contains("body_collider_center"))
    {
        const auto& center = data["body_collider_center"];
        _bodyColliderCenter = Vec3(center[0].get<float>(), center[1].get<float>(), center[2].get<float>());
    }

    if (data.contains("body_collider_extents"))
    {
        const auto& extents = data["body_collider_extents"];
        _bodyColliderExtents = Vec3(extents[0].get<float>(), extents[1].get<float>(), extents[2].get<float>());
    }

    if (data.contains("network_driven"))
        _networkDriven = data["network_driven"].get<bool>();

    if (data.contains("hit_effect_asset_name"))
        _hitEffectAssetName = data["hit_effect_asset_name"].get<string>();

    if (data.contains("idle_state_name"))
        _idleStateName = data["idle_state_name"].get<string>();
}

HRESULT Boss_Pain::Ready_Components()
{
    CHECK_FAILED(Character::Ready_Components(), E_FAIL);

    {
        CombatStat::FCombatStatDesc statDesc{};
        statDesc.maxHp = _maxHp;
        statDesc.attack = _attack;

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COMBAT_STAT, _combatStat, &statDesc), E_FAIL);
    }

    {
        MovementComponent::FMovementDesc movementDesc{};
        movementDesc.maxWalkSpeed = _maxWalkSpeed;
        movementDesc.maxSprintSpeed = _maxSprintSpeed;

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_MOVEMENT, _movement, &movementDesc), E_FAIL);
    }

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_AI_CONTROLLER, _aiController), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_BEHAVIOR, _behavior), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_ANIMATION_STATE, _animState), E_FAIL);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXANIMMESH, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(_modelComponentID, _model), E_FAIL);

    {
        Bounding_OBB::FBoundingOBBDesc obbDesc{};
        obbDesc.center = _bodyColliderCenter;
        obbDesc.extents = _bodyColliderExtents;
        obbDesc.radians = Vec3::Zero;

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COLLIDER_OBB, _collider, &obbDesc), E_FAIL);
        _collider->Set_CollisionPreset(Collision_Preset::Monster_Body);
    }

    return S_OK;
}

Protocol::OBJECT_TYPE Boss_Pain::Get_EnemyObjectType() const
{
    return Protocol::OBJECT_TYPE_BOSS_PAIN;
}

Protocol::OBJECT_STATE_TYPE Boss_Pain::To_EnemyObjectState(const string& animStateName) const
{
    if (animStateName == "Run" || animStateName.rfind("Run_", 0) == 0)
        return Protocol::OBJECT_STATE_TYPE_RUN;

    if (animStateName == "Attack_04")
        return Protocol::OBJECT_STATE_TYPE_ATTACK_04;

    if (animStateName == "Attack_03")
        return Protocol::OBJECT_STATE_TYPE_ATTACK_03;

    if (animStateName == "Attack_02")
        return Protocol::OBJECT_STATE_TYPE_ATTACK_02;

    if (animStateName == "Attack_01" || animStateName == "Attack_1")
        return Protocol::OBJECT_STATE_TYPE_ATTACK_01;

    if (animStateName == "Attack" || animStateName.rfind("Attack_", 0) == 0)
        return Protocol::OBJECT_STATE_TYPE_ATTACK;

    if (animStateName == "Hit" || animStateName.rfind("Hit_", 0) == 0)
        return Protocol::OBJECT_STATE_TYPE_HIT;

    if (animStateName == "Dead" || animStateName == "Die" ||
        animStateName.rfind("Dead_", 0) == 0 || animStateName.rfind("Die_", 0) == 0)
    {
        return Protocol::OBJECT_STATE_TYPE_DEAD;
    }

    return Protocol::OBJECT_STATE_TYPE_IDLE;
}

Shared<Boss_Pain> Boss_Pain::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Boss_Pain>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Boss_Pain");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> Boss_Pain::Clone(void* arg)
{
    auto clone = make_shared<Boss_Pain>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Boss_Pain");
        return nullptr;
    }

    return clone;
}

void Boss_Pain::Free()
{
    EnemyCharacter::Free();
}

NS_END
