#include "pch.h"
#include "UI_PlayerHP.h"
#include "GameInstance.h"
#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"

UI_PlayerHP::UI_PlayerHP(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
}

UI_PlayerHP::UI_PlayerHP(const UI_PlayerHP& rhs)
    : UIObject(rhs)
    , _hpRatio(rhs._hpRatio)
{
}

HRESULT UI_PlayerHP::Initialize_Prototype()
{
    return UIObject::Initialize_Prototype();
}

HRESULT UI_PlayerHP::Initialize(void* arg)
{
    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);

    CHECK_FAILED(Ready_Components(), E_FAIL);

    _transformCom->Set_LocalScale(200.f, 20.f, 1.f);

    return S_OK;
}

void UI_PlayerHP::Priority_Update(float timeDelta)
{
    UIObject::Priority_Update(timeDelta);
}

void UI_PlayerHP::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    __super::Update_Transform();
}

void UI_PlayerHP::Late_Update(float timeDelta)
{
    UIObject::Late_Update(timeDelta);

}

HRESULT UI_PlayerHP::Render()
{
    if (!_isVisible) return S_OK;

    _shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix);
    __super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View);
    __super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj);

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", 1), E_FAIL);

    // CHECK_FAILED(_shaderCom->Bind_RawValue("g_Ratio", &_hpRatio, sizeof(float)), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

HRESULT UI_PlayerHP::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_PLAYER_STATUS, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

Shared<UI_PlayerHP> UI_PlayerHP::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, void* arg)
{
    auto instance = make_shared<UI_PlayerHP>(device, context);

    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Failed to Created : UI_PlayerHP");
        return nullptr;
    }

    return instance;
}

void UI_PlayerHP::Free()
{
    UIObject::Free();

}
