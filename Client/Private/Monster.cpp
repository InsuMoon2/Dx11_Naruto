#include "pch.h"
#include "Monster.h"
#include "CombatStat.h"
#include "MovementComponent.h"
#include "AIController.h"
#include "BehaviorTree.h"
#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"

Monster::Monster(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Character(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_MONSTER);
}

Monster::Monster(const Monster& rhs)
    : Character(rhs)
{
}

Monster::~Monster()
{
}

HRESULT Monster::Initialize_Prototype()
{
    return Character::Initialize_Prototype();
}

HRESULT Monster::Initialize(void* arg)
{
    return Character::Initialize(arg);
}

void Monster::BeginPlay()
{
    Character::BeginPlay();
}

void Monster::Priority_Update(float timeDelta)
{
    Character::Priority_Update(timeDelta);
}

void Monster::Update(float timeDelta)
{
    Character::Update(timeDelta);

    _aiController->Update(timeDelta);
}

void Monster::Late_Update(float timeDelta)
{
    Character::Late_Update(timeDelta);

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, this->GetSharedPtr());
}

HRESULT Monster::Render()
{
    Character::Render();

    Matrix worldMatrix = _transformCom->Get_WorldMatrix();
    _shaderCom->Bind_Matrix("g_WorldMatrix", &worldMatrix);
    _shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View));
    _shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj));

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", _textureCom->Get_CurrentIndex()), E_FAIL);
    CHECK_FAILED(_shaderCom->Begin(0), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

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
        CombatStat::FCombatStatDesc desc;
        desc.maxHp = 100.f;
        desc.attack = 10.f;

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COMBAT_STAT, _combatStat, &desc), E_FAIL);
    }

    {
        MovementComponent::FMovementDesc desc;
        desc.maxWalkSpeed = 2.f;
        desc.maxSprintSpeed = 4.f;

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_MOVEMENT, _movement, &desc), E_FAIL);
    }

    // AI
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_AI_CONTROLLER, _aiController), E_FAIL);;
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_AI, _behavior), E_FAIL);

    _transformCom->Set_LocalPosition(0.f, 0.f, -5.f);

    // Temp : 렌더링
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXTEX, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

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
    Character::Free();
}
