#include "pch.h"
#include "Player.h"
#include "CombatStat.h"
#include "MovementComponent.h"
#include "PlayerController.h"
#include "InputComponent.h"

#include "Texture.h"
#include "Shader.h"
#include "Model.h"
#include "VIBuffer_Rect.h"

#include "NetworkManager.h"

Player::Player(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Character(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_PLAYER);
}

Player::Player(const Player& rhs)
    : Character(rhs)
{

}

HRESULT Player::Initialize_Prototype()
{

    return S_OK;
}

HRESULT Player::Initialize(void* arg)
{
    CHECK_FAILED(Character::Initialize(arg), E_FAIL);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXMESH, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_MODEL_PLAYER, _model), E_FAIL);

    return S_OK;
}

void Player::BeginPlay()
{
    Character::BeginPlay();


}

void Player::Priority_Update(float timeDelta)
{
    Character::Priority_Update(timeDelta);
}

void Player::Update(float timeDelta)
{
    Character::Update(timeDelta);

}

void Player::Late_Update(float timeDelta)
{
    Character::Late_Update(timeDelta);

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, this->GetSharedPtr());
}

HRESULT Player::Render()
{
    Character::Render();

    size_t numMeshes = _model->Get_NumMeshes();
    for (size_t i = 0; i < numMeshes; i++)
    {
        _model->Bind_Material(_shaderCom, "g_DiffuseTexture", i, EMaterialTextureSlot::BaseColor, 0);

        CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(_model->Render(i), E_FAIL);
    }

    return S_OK;
}

void Player::Sync(const Protocol::ObjectInfo& info)
{
    _transformCom->Set_LocalPosition(info.pos().x(), info.pos().y(), info.pos().z());
}

HRESULT Player::Ready_Components()
{
    Character::Ready_Components();


    return S_OK;
}

HRESULT Player::Bind_ShaderResources()
{
    Matrix worldMatrix = _transformCom->Get_WorldMatrix();
    _shaderCom->Bind_Matrix("g_WorldMatrix", &worldMatrix);
    _shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View));
    _shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj));

    GAME->Bind_CamPosition(_shaderCom, "g_CamPosition");

    CHECK_FAILED(Bind_Lights(), E_FAIL);

    return S_OK;
}

HRESULT Player::Bind_Lights()
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

    _shaderCom->Bind_RawValue("g_LightDir", &lightDesc->direction, sizeof(Vec4));
    _shaderCom->Bind_RawValue("g_LightDiffuse", &lightDesc->diffuse, sizeof(Vec4));
    _shaderCom->Bind_RawValue("g_LightAmbient", &lightDesc->ambient, sizeof(Vec4));
    _shaderCom->Bind_RawValue("g_LightSpecular", &lightDesc->specular, sizeof(Vec4));
}

shared_ptr<GameObject> Player::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Player>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        return nullptr;
    }

    return instance;
}

shared_ptr<GameObject> Player::Clone(void* arg)
{
    auto instance = make_shared<Player>(*this);

    if (FAILED(instance->Initialize(arg)))
    {
        return nullptr;
    }

    return instance;
}
