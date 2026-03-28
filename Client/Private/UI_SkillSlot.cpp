#include "pch.h"
#include "UI_SkillSlot.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"
#include "Texture.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(UI_SkillSlot, Protocol::OBJECT_TYPE_UI_SKILL_SLOT)

UI_SkillSlot::UI_SkillSlot(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
    
}

UI_SkillSlot::UI_SkillSlot(const UI_SkillSlot& rhs)
    : UIObject(rhs)
    , _baseSrvIndex(rhs._baseSrvIndex)
    , _maskSrvIndex(rhs._maskSrvIndex)
    , _iconSrvIndex(rhs._iconSrvIndex)
    , _cooldownRatio(rhs._cooldownRatio)
{
}

HRESULT UI_SkillSlot::Initialize_Prototype()
{
    CHECK_FAILED(UIObject::Initialize_Prototype(), E_FAIL);


    return S_OK;
}

HRESULT UI_SkillSlot::Initialize(void* arg)
{
    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FSkillSlotDesc*>(arg);
    if (desc)
    {
        _baseSrvIndex = desc->baseSrvIndex;
        _maskSrvIndex = desc->maskSrvIndex;
        _iconSrvIndex = desc->iconSrvIndex;

        _textureComponentType = desc->textureComponentType;
    }

    CHECK_FAILED(Ready_Components(), E_FAIL);

    return S_OK;
}

void UI_SkillSlot::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    __super::Update_Transform();
}

HRESULT UI_SkillSlot::Render()
{
    if (!_isVisible)
        return S_OK;

    CHECK_FAILED(Bind_CommonShaderResources(), E_FAIL);

    CHECK_FAILED(Render_Base(), E_FAIL);
    CHECK_FAILED(Render_MaskedIcon(), E_FAIL);

    if (_cooldownRatio > 0.001f)
    {
        CHECK_FAILED(Render_MaskedCooldwn(), E_FAIL);
    }


    return S_OK;
}

HRESULT UI_SkillSlot::Bind_CommonShaderResources()
{
    CHECK_NULL(_shaderCom, E_FAIL);
    CHECK_NULL(_gaugeTextureCom, E_FAIL);
    CHECK_NULL(_iconTextureCom, E_FAIL);
    CHECK_NULL(_bufferCom, E_FAIL);

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    return S_OK;
}

HRESULT UI_SkillSlot::Render_Base()
{
    // 배경/게이지 베이스
    float alpha = 1.f;

    Matrix baseScaleMatrix = Matrix::CreateScale(1.18f, 1.18f, 1.f);
    Matrix baseWorldMatrix = baseScaleMatrix * _worldMatrix;

    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &baseWorldMatrix), E_FAIL);
    CHECK_FAILED(_gaugeTextureCom->Bind_SRV(_shaderCom, "g_Texture", _baseSrvIndex), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

HRESULT UI_SkillSlot::Render_MaskedIcon()
{
    // 실제 스킬 아이콘
    float alpha = 1.f;

    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix), E_FAIL);

    CHECK_FAILED(_iconTextureCom->Bind_SRV(_shaderCom, "g_Texture", _iconSrvIndex), E_FAIL);

    // 원형 마스크 역할을 할 Base 텍스처도 세팅
    CHECK_FAILED(_gaugeTextureCom->Bind_SRV(_shaderCom, "g_MaskTexture", _maskSrvIndex), E_FAIL);

    // 마스크용 패스 사용
    CHECK_FAILED(_shaderCom->Begin_Pass(5), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

HRESULT UI_SkillSlot::Render_MaskedCooldwn()
{
    // 쿨타임
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_CooldownRatio", &_cooldownRatio, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_CooldownOverlayAlpha", &_cooldownOverlayAlpha, sizeof(float)), E_FAIL);

    CHECK_FAILED(_iconTextureCom->Bind_SRV(_shaderCom, "g_Texture", _iconSrvIndex), E_FAIL);

    CHECK_FAILED(_gaugeTextureCom->Bind_SRV(_shaderCom, "g_MaskTexture", _maskSrvIndex), E_FAIL);

    // 마스크 적용 쿨타임 패스
    CHECK_FAILED(_shaderCom->Begin_Pass(6), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

HRESULT UI_SkillSlot::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_SKILL_GAUGE, _gaugeTextureCom), E_FAIL);
    CHECK_FAILED(Add_Component(_textureComponentType, _iconTextureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

Shared<UI_SkillSlot> UI_SkillSlot::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_SkillSlot>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Prototype : UI_SkillSlot");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_SkillSlot::Clone(void* arg)
{
    auto clone = make_shared<UI_SkillSlot>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : UI_SkillSlot");

        return nullptr;
    }

    return clone;
}

void UI_SkillSlot::Free()
{
    UIObject::Free();
}
