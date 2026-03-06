#include "pch.h"
#include "UI_PlayerStatus.h"

#include "Shader.h"
#include "Texture.h"
#include "VIBuffer_Rect.h"

#include "Player.h"
#include "UI_PlayerHP.h"

UI_PlayerStatus::UI_PlayerStatus(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
}

UI_PlayerStatus::UI_PlayerStatus(const UI_PlayerStatus& rhs)
    : UIObject(rhs)
{
}

HRESULT UI_PlayerStatus::Initialize_Prototype()
{
    return UIObject::Initialize_Prototype();
}

HRESULT UI_PlayerStatus::Initialize(void* arg)
{
    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);
    CHECK_FAILED(Ready_Components(), E_FAIL);

    UI_PlayerHP::FPlayerHPDesc hpDesc;
    hpDesc.posX = 0.f;
    hpDesc.posY = 0.f;
    hpDesc.sizeX = 200.f;
    hpDesc.sizeY = 20.f;
    hpDesc.zOrder = _zOrder;
    hpDesc.levelIndex = _levelIndex;
    hpDesc.textureIndex = 0;
    hpDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT;

    _hpBar = Create_Child<UI_PlayerHP>(EUILayer::HUD, &hpDesc);
    _hpBar->Get_Transform()->Set_LocalPosition(50.f, 0.f, _zOrder);


    return S_OK;
}

void UI_PlayerStatus::Priority_Update(float timeDelta)
{
    UIObject::Priority_Update(timeDelta);
}

void UI_PlayerStatus::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    __super::Update_Transform();

    /*if (auto pPlayer = _player.lock())
    {
        _hpBar->Set_Ratio(pPlayer->Get_CurrentHP() / pPlayer->Get_MaxHP());
    }*/
}

void UI_PlayerStatus::Late_Update(float timeDelta)
{
    UIObject::Late_Update(timeDelta);
}

HRESULT UI_PlayerStatus::Render()
{
    _shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix);

    __super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View);
    __super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj);

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", 0), E_FAIL);
    CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

HRESULT UI_PlayerStatus::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_PLAYER_STATUS, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXTEX, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

Shared<UI_PlayerStatus> UI_PlayerStatus::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, void* arg)
{
    auto instance = make_shared<UI_PlayerStatus>(device, context);

    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Faield to Created : UI_PlayerStatus");
        instance->Free();

        return nullptr;
    }

    return instance;
}
