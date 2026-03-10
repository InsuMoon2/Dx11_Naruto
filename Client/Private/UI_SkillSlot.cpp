#include "pch.h"
#include "UI_SkillSlot.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"
#include "Texture.h"

UI_SkillSlot::UI_SkillSlot(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
}

UI_SkillSlot::UI_SkillSlot(const UI_SkillSlot& rhs)
    : UIObject(rhs)
    , _baseSrvIndex(rhs._baseSrvIndex)
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
    CHECK_FAILED(Ready_Components(), E_FAIL);

    auto* desc = static_cast<FSkillSlotDesc*>(arg);
    if (desc)
    {
        _baseSrvIndex = desc->baseSrvIndex;
        _iconSrvIndex = desc->iconSrvIndex;
    }

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

    _shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix);
    __super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View);
    __super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj);

    // 아이콘 투명 일단 임시
    {
        float alpha = 0.35f + (1.f - _cooldownRatio) * 0.65f;
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);
        CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", _iconSrvIndex), E_FAIL);
        CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
        CHECK_FAILED(_bufferCom->Render(), E_FAIL);
    }

    {
        float alpha = 1.f;
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);
        CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", _baseSrvIndex), E_FAIL);
        CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
        CHECK_FAILED(_bufferCom->Render(), E_FAIL);
    }
   

    return S_OK;
}

HRESULT UI_SkillSlot::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_SKILL_ICON, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

Shared<UI_SkillSlot> UI_SkillSlot::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, void* arg)
{
    auto instance = make_shared<UI_SkillSlot>(device, context);

    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Failed to Created : UI_SkillSlot");
        return nullptr;
    }

    return instance;
}

void UI_SkillSlot::Free()
{
    UIObject::Free();
}
