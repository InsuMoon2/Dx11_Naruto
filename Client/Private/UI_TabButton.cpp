#include "pch.h"
#include "UI_TabButton.h"
#include "GameObject_Factory.h"
#include "UI_Text.h"
#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"
#include "GameInstance.h"
#include "Level_CharacterSetup.h"

REGISTER_GAMEOBJECT(UI_TabButton, Protocol::OBJECT_TYPE_UI_TAB)

UI_TabButton::UI_TabButton(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
}

UI_TabButton::UI_TabButton(const UI_TabButton& rhs)
    : UIObject(rhs)
{
}

HRESULT UI_TabButton::Initialize(void* arg)
{
    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);

    const auto* desc = static_cast<FUITabDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);

    CHECK_FAILED(Ready_Components(), E_FAIL);
    CHECK_FAILED(Ready_ChildText(desc), E_FAIL);

    Set_Selected(false);

    return S_OK;
}

void UI_TabButton::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    __super::Update_Transform();
}

void UI_TabButton::Late_Update(float timeDelta)
{
    UIObject::Late_Update(timeDelta);
}

HRESULT UI_TabButton::Render()
{
    if (!_isVisible) return S_OK;

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", ETOI(ECharacterSetupTexture::TabButton)), E_FAIL);

    float alpha = _opacity;
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL); // UI 기본용 패스 확인 필요

    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

void UI_TabButton::Set_Selected(bool bSelected)
{
    // 이벤트 넘겨줘야함
}

void UI_TabButton::Set_Visibility(bool active)
{
    UIObject::Set_Visibility(active);

    if (_labelUI)
        _labelUI->Set_Visibility(active);
}

HRESULT UI_TabButton::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

HRESULT UI_TabButton::Ready_ChildText(const FUITabDesc* desc)
{
    // 텍스트 출력
    {
        UI_Text::FUITextDesc textDesc{};
        textDesc.name = format(L"{} Label", desc->name);

        textDesc.posX = desc->posX + desc->labelOffset.x;
        textDesc.posY = desc->posY + desc->labelOffset.y;

        textDesc.sizeX = (desc->labelSize.x > 0.f) ? desc->labelSize.x : desc->sizeX;
        textDesc.sizeY = (desc->labelSize.y > 0.f) ? desc->labelSize.y : desc->sizeY;

        textDesc.zOrder = _zOrder + 0.01f;
        textDesc.levelIndex = desc->levelIndex;

        textDesc.text = desc->labelText;
        textDesc.style.fontFamily = L"Malgun Gothic";
        textDesc.style.fontSize = desc->fontSize;
        textDesc.style.color = Color(0.f, 0.f, 0.f, 1.f);
        textDesc.style.hAlign = ETextHAlign::Center;
        textDesc.style.vAlign = ETextVAlign::Middle;
        textDesc.style.wordWrap = false;

        _labelUI = static_pointer_cast<UI_Text>(
            GAME->Add_UI(Protocol::OBJECT_TYPE_UI_TEXT, EUILayer::HUD, &textDesc));
        CHECK_NULL(_labelUI, E_FAIL);
    }

    return S_OK;
}

Shared<UI_TabButton> UI_TabButton::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_TabButton>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Prototype : UI_TabButton");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_TabButton::Clone(void* arg)
{
    auto clone = make_shared<UI_TabButton>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : UI_TabButton");
        return nullptr;
    }

    return clone;
}

void UI_TabButton::Free()
{
    UIObject::Free();
}
