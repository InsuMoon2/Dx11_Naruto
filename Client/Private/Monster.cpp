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

#include "Bounding_Capsule.h"
#include "Collider.h"

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
    _model->Play_Animation(timeDelta);
}

void Monster::Late_Update(float timeDelta)
{
    Character::Late_Update(timeDelta);

    if (_collider)
        _collider->Update_Collider(_transformCom->Get_WorldMatrix());

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, this->GetSharedPtr());
}

HRESULT Monster::Render()
{
    Character::Render();

    if (!_model)
        return E_FAIL;

    size_t numMeshes = _model->Get_NumMeshes();

    for (size_t i = 0; i < numMeshes; i++)
    {
        CHECK_FAILED(_model->Bind_BoneMatrices(_shaderCom, "g_BoneMatrices"), E_FAIL);
        _model->Bind_Material(_shaderCom, "g_DiffuseTexture", i, EMaterialTextureSlot::BaseColor, 0);

        _shaderCom->Begin_Pass(0);
        _model->Render(i);
    }

    return S_OK;
}

HRESULT Monster::Bind_Lights()
{
    const FLightDesc* lightDesc = GAME->Get_LightDesc(0);

    FLightDesc defaultLight;
    if (!lightDesc)
    {
        defaultLight.direction = Vec4(0.f, -1.f, 1.f, 0.f);
        defaultLight.diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
        defaultLight.ambient = Vec4(0.4f, 0.4f, 0.4f, 1.f);
        defaultLight.specular = Vec4(1.f, 1.f, 1.f, 1.f);
        lightDesc = &defaultLight;
    }

    CHECK_NULL(lightDesc, E_FAIL);

    _shaderCom->Bind_RawValue("g_vLightDir", &lightDesc->direction, sizeof(Vec4));
    _shaderCom->Bind_RawValue("g_vLightDiffuse", &lightDesc->diffuse, sizeof(Vec4));
    _shaderCom->Bind_RawValue("g_vLightAmbient", &lightDesc->ambient, sizeof(Vec4));
    _shaderCom->Bind_RawValue("g_vLightSpecular", &lightDesc->specular, sizeof(Vec4));
}

void Monster::OnBeginOverlap(Shared<Collider> other)
{
    Character::OnBeginOverlap(other);
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
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_BEHAVIOR, _behavior), E_FAIL);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_ANIMATION_STATE, _animState), E_FAIL);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXANIMMESH, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_MODEL_MONSTER, _model), E_FAIL);

    // 충돌체 추가
    Bounding_Capsule::FBoundingCapsuleDesc capsuleDesc{};
    capsuleDesc.radius = 0.5f;
    capsuleDesc.halfHeight = 0.3f;

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COLLIDER_CAPSULE, _collider, &capsuleDesc), E_FAIL);
    _collider->Set_CollisionPreset(Collision_Preset::Monster);

    GAME->Add_Collider(_collider);

    return S_OK;
}

HRESULT Monster::Bind_ShaderResources()
{
    Matrix worldMatrix = _transformCom->Get_WorldMatrix();
    _shaderCom->Bind_Matrix("g_WorldMatrix", &worldMatrix);
    _shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View));
    _shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj));

    GAME->Bind_CamPosition(_shaderCom, "g_vCamPosition");

    CHECK_FAILED(Bind_Lights(), E_FAIL);

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
