#include "pch.h"
#include "Monster.h"

#include "CombatStat.h"
#include "MovementComponent.h"
#include "AIController.h"
#include "BehaviorTree.h"
#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"
#include "Model.h"
#include "AnimationStateComponent.h"
#include "GameObject_Factory.h"
#include "Bounding_Sphere.h"
#include "Collider.h"
#include "UI_MonsterHp.h"

REGISTER_GAMEOBJECT(Monster, Protocol::OBJECT_TYPE_MONSTER)

IMPLEMENT_REFLECTION(Monster);

bool Monster::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "Monster";

    return true;
}

NS_BEGIN(Client)

Monster::Monster(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : EnemyCharacter(device, context)
{
}

Monster::Monster(const Monster& rhs)
    : EnemyCharacter(rhs)
{
}

Monster::~Monster()
{
}

HRESULT Monster::Initialize_Prototype()
{
    CHECK_FAILED(EnemyCharacter::Initialize_Prototype(), E_FAIL);

    return S_OK;
}

HRESULT Monster::Initialize(void* arg)
{
    CHECK_FAILED(EnemyCharacter::Initialize(arg), E_FAIL);
    CHECK_FAILED(Ready_UI(), E_FAIL);

    return S_OK;
}

json Monster::To_Json() const
{
    json j = Character::To_Json();

    return j;
}

void Monster::From_Json(const json& data)
{
    Character::From_Json(data);
}

HRESULT Monster::Ready_Components()
{
    {
        CombatStat::FCombatStatDesc desc{};
        desc.maxHp = 100.f;
        desc.attack = 10.f;

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COMBAT_STAT, _combatStat, &desc), E_FAIL);
    }

    {
        MovementComponent::FMovementDesc desc{};
        desc.maxWalkSpeed = 2.f;
        desc.maxSprintSpeed = 4.f;

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_MOVEMENT, _movement, &desc), E_FAIL);
    }

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_AI_CONTROLLER, _aiController), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_BEHAVIOR, _behavior), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_ANIMATION_STATE, _animState), E_FAIL);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXANIMMESH, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_MODEL_MONSTER, _model), E_FAIL);

    Bounding_Sphere::FBoundingSphereDesc sphereDesc{};
    sphereDesc.radius = 1.5f;

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COLLIDER_SPHERE, _collider, &sphereDesc), E_FAIL);
    _collider->Set_CollisionPreset(Collision_Preset::Monster_Body);

    return S_OK;
}

Protocol::OBJECT_TYPE Monster::Get_EnemyObjectType() const
{
    return Protocol::OBJECT_TYPE_MONSTER;
}

Protocol::OBJECT_STATE_TYPE Monster::To_EnemyObjectState(const string& animStateName) const
{
    if (animStateName == "Run" || animStateName.rfind("Run_", 0) == 0)
        return Protocol::OBJECT_STATE_TYPE_RUN;

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

HRESULT Monster::Ready_UI()
{
    UI_MonsterHp::FPlayerHPDesc hpDesc{};
    hpDesc.posX = 0.f;
    hpDesc.posY = 0.f;
    hpDesc.zOrder = 0.5f;
    hpDesc.levelIndex = _levelIndex;
    hpDesc.textureIndex = 0;
    hpDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT;

    Shared<UIObject> uiObj = GAME->Add_UI(Protocol::OBJECT_TYPE_UI_MONSTER_HP, EUILayer::HUD, &hpDesc);
    _hpBar = static_pointer_cast<UI_MonsterHp>(uiObj);

    if (_hpBar)
    {
        _hpBar->Set_FillRange(98.f / 512.f, 413.f / 512.f);
        _hpBar->Bind_Monster(GetSharedPtr<Monster>());
    }

    return S_OK;
}

Shared<Monster> Monster::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Monster>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : Monster");
        return nullptr;
    }

    return instance;
}

shared_ptr<GameObject> Monster::Clone(void* arg)
{
    auto clone = make_shared<Monster>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : Monster");
        return nullptr;
    }

    return clone;
}

void Monster::Free()
{
    EnemyCharacter::Free();
}

NS_END
