#include "pch.h"
#include "Player.h"
#include "CombatStat.h"
#include "MovementComponent.h"
#include "PlayerController.h"
#include "InputComponent.h"

#include "Texture.h"
#include "Shader.h"
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

    _transformCom->Set_LocalPosition(0.f, 0.f, -5.f);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXTEX, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

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

    Matrix worldMatrix = _transformCom->Get_WorldMatrix();
    _shaderCom->Bind_Matrix("g_WorldMatrix", &worldMatrix);
    GAME->Bind_TransformMatrix(ETransformState::View, _shaderCom, "g_ViewMatrix");
    GAME->Bind_TransformMatrix(ETransformState::Proj, _shaderCom, "g_ProjMatrix");

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", _textureCom->Get_CurrentIndex()), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin(0), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

void Player::Sync(const Protocol::ObjectInfo& info)
{
    // TODO : Rotation도 추가 예정?
    _transformCom->Set_LocalPosition(info.pos().x(), info.pos().y(), info.pos().z());
}

HRESULT Player::Ready_Components()
{
    Character::Ready_Components();


    return S_OK;
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
