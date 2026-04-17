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
#include "Blackboard.h"
#include "Bounding_Sphere.h"
#include "Collider.h"
#include "UI_MonsterHp.h"

REGISTER_GAMEOBJECT(Monster, Protocol::OBJECT_TYPE_MONSTER)

IMPLEMENT_REFLECTION(Monster);

bool Monster::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "Monster";

    PROPERTY_FLOAT("Test Value : ", _test, 1.f, 9999.f);

    return true;
}

Monster::Monster(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Character(device, context)
{
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
    CHECK_FAILED(Character::Initialize_Prototype(), E_FAIL);

    return S_OK;
}

HRESULT Monster::Initialize(void* arg)
{
    CHECK_FAILED(Character::Initialize(arg), E_FAIL);
    
    CHECK_FAILED(Ready_UI(), E_FAIL);

    return S_OK;
}

void Monster::BeginPlay()
{
    Character::BeginPlay();

    auto animState = Get_Component<AnimationStateComponent>();
    if (animState)
    {
        animState->Play_State("Idle");

        auto model = Get_Component<Model>();
        if (model)
            model->Play_Animation(0.f); 
    }
}

void Monster::Priority_Update(float timeDelta)
{
    Character::Priority_Update(timeDelta);
}

void Monster::Update(float timeDelta)
{
    Character::Update(timeDelta);

    if (!_networkDriven && _aiController)
    {
        _aiController->Update(timeDelta);
    }

    if (_model)
    {
        _model->Play_Animation(timeDelta);
    }
}

void Monster::Late_Update(float timeDelta)
{
    Character::Late_Update(timeDelta);

    if (_collider)
    {
        _collider->Update_Collider(_transformCom->Get_WorldMatrix());
        GAME->Add_Collider(_collider);
    }

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, this->GetSharedPtr());
}

HRESULT Monster::Render()
{
    if (!_model || !_shaderCom || Is_Destroy())
        return S_OK;

    CHECK_FAILED(Character::Render(), E_FAIL);

    const size_t numMeshes = _model->Get_NumMeshes();
    if (numMeshes == 0)
        return S_OK;

    if (FAILED(_model->Bind_BoneMatrices(_shaderCom, "g_BoneMatrices")))
        return S_OK;

    for (size_t i = 0; i < numMeshes; i++)
    {
        _model->Bind_Material(_shaderCom, "g_DiffuseTexture", i, EMaterialTextureSlot::BaseColor, 0);

        CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(_model->Render(static_cast<uint32>(i)), E_FAIL);
    }

    return S_OK;
}

void Monster::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    Character::OnBeginOverlap(self, other);


}

void Monster::TakeDamage(const FDamageEvent& damageEvent)
{
    Character::TakeDamage(damageEvent);

    if (_combatStat && _combatStat->Is_Dead())
        return;

    if (_combatStat)
        _combatStat->Take_Damage(damageEvent);


}

void Monster::OnDamaged(const FDamageEvent& damageEvent)
{
    Character::OnDamaged(damageEvent);

    if (_behavior)
    {
        auto blackboard = _behavior->Get_Blackboard();
        if (blackboard)
        {
            blackboard->Set_ValueAsBool("IsHit", true);
        }
    }
}

void Monster::OnDead(const FDamageEvent& damageEvent)
{
    Character::OnDead(damageEvent);

    // TODO : 상태 전환 -> 몬스터는 비헤이비어 트리에서 상태값 변경해주기
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

void Monster::Sync(const Protocol::ObjectInfo& info)
{
    _transformCom->Set_WorldPosition(info.pos().x(), info.pos().y(), info.pos().z());
    _transformCom->Set_LocalRotation(0.f, info.rot_y(), 0.f);
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
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_BEHAVIOR, _behavior), E_FAIL);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_ANIMATION_STATE, _animState), E_FAIL);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXANIMMESH, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_MODEL_MONSTER, _model), E_FAIL);

    // 충돌체 추가
    Bounding_Sphere::FBoundingSphereDesc sphereDesc{};
    sphereDesc.radius = 1.5f;

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COLLIDER_SPHERE, _collider, &sphereDesc), E_FAIL);
    _collider->Set_CollisionPreset(Collision_Preset::Monster_Body);

    return S_OK;
}

HRESULT Monster::Bind_ShaderResources()
{
    Matrix worldMatrix = _transformCom->Get_WorldMatrix();
    _shaderCom->Bind_Matrix("g_WorldMatrix", &worldMatrix);
    _shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View));
    _shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj));

    return S_OK;
}

HRESULT Monster::Ready_UI()
{
    UI_MonsterHp::FPlayerHPDesc hpDesc;
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
    Character::Free();
}
