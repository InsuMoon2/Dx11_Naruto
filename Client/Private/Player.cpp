#include "pch.h"
#include "Player.h"
#include "CombatStat.h"
#include "MovementComponent.h"
#include "PlayerController.h"
#include "InputComponent.h"

#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"

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

    {
        CombatStat::FCombatStatDesc desc;
        desc.maxHp = 200.f;
        desc.attack = 100.f;

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COMBAT_STAT, _combatStat, &desc), E_FAIL);
    }

    {
        MovementComponent::FMovementDesc moveDesc;
        moveDesc.maxWalkSpeed = 4.f;
        moveDesc.maxSprintSpeed = 7.f;

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_MOVEMENT, _movement, &moveDesc), E_FAIL);
        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_INPUT, _input), E_FAIL);
    }
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_PLAYER_CONTROLLER, _playerController), E_FAIL);
    
    _transformCom->Set_LocalPosition(0.f, 0.f, -5.f);

    // Temp : 렌더링
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

    //_playerController->Update(timeDelta);
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
    _shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View));
    _shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj));

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", _textureCom->Get_CurrentIndex()), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin(0), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
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
