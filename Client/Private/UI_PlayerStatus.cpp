#include "pch.h"
#include "UI_PlayerStatus.h"
#include "Shader.h"
#include "Texture.h"
#include "VIBuffer_Rect.h"

#include "Player.h"
#include "UI_PlayerHP.h"

UI_PlayerStatus::UI_PlayerStatus(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Panel(device, context)
{
}

UI_PlayerStatus::UI_PlayerStatus(const UI_PlayerStatus& rhs)
    : Panel(rhs)
{
}

HRESULT UI_PlayerStatus::Initialize_Prototype()
{
    return Panel::Initialize_Prototype();
}

HRESULT UI_PlayerStatus::Initialize(void* arg)
{
    CHECK_FAILED(Panel::Initialize(arg), E_FAIL);
    CHECK_FAILED(Ready_Components(), E_FAIL);

    UI_PlayerHP::FPlayerHPDesc hpDesc;
    hpDesc.posX = 0.f;
    hpDesc.posY = 0.f;
    hpDesc.sizeX = 255.f;
    hpDesc.sizeY = 20.f;
    hpDesc.zOrder = _zOrder;
    hpDesc.levelIndex = _levelIndex;
    hpDesc.textureIndex = 0;
    hpDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT;

    _hpBar = Create_Child<UI_PlayerHP>(EUILayer::HUD, &hpDesc);
    if (!_hpBar) return E_FAIL;

    _hpBar->Get_Transform()->Set_LocalPosition(50.f, 0.f, _zOrder);


    return S_OK;
}

void UI_PlayerStatus::Priority_Update(float timeDelta)
{
    Panel::Priority_Update(timeDelta);
}

void UI_PlayerStatus::Update(float timeDelta)
{
    Panel::Update(timeDelta);

    __super::Update_Transform();

    /*if (auto pPlayer = _player.lock())
    {
        _hpBar->Set_Ratio(pPlayer->Get_CurrentHP() / pPlayer->Get_MaxHP());
    }*/
}

void UI_PlayerStatus::Late_Update(float timeDelta)
{
    Panel::Late_Update(timeDelta);
}

HRESULT UI_PlayerStatus::Render()
{
    if (!_isVisible) return S_OK;

    _shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix);

    Panel::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View);
    Panel::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj);

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", 0), E_FAIL);

#pragma region Alpha값 테스트
    float fAlpha = 0.5f;
    _shaderCom->Bind_RawValue("g_Alpha", &fAlpha, sizeof(float));
#pragma endregion

    CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

HRESULT UI_PlayerStatus::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_PLAYER_STATUS, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
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

void UI_PlayerStatus::Free()
{
    Panel::Free();

}
