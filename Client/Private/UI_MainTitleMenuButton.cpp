#include "pch.h"
#include "UI_MainTitleMenuButton.h"
#include "UI_Text.h"
#include "Level_MainTitle.h"
#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"
#include "GameInstance.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(UI_MainTitleMenuButton, Protocol::OBJECT_TYPE_UI_MAIN_TITLE_TEXT)

UI_MainTitleMenuButton::UI_MainTitleMenuButton(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
    
}

UI_MainTitleMenuButton::UI_MainTitleMenuButton(const UI_MainTitleMenuButton& rhs)
    : UIObject(rhs)
{
}

HRESULT UI_MainTitleMenuButton::Initialize(void* arg)
{
    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);

    const auto* desc = static_cast<FMainTitleMenuDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);

    CHECK_FAILED(Ready_Components(), E_FAIL);
    CHECK_FAILED(Ready_ChildText(desc), E_FAIL);

    // 선택됐을 때 크기 조절하게 백업용
    _baseScaleX = _transformCom->Get_LocalScale().x;
    _baseScaleY = _transformCom->Get_LocalScale().y;

    Set_Selected(false);

    return S_OK;
}

void UI_MainTitleMenuButton::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    __super::Update_Transform();
}

void UI_MainTitleMenuButton::Late_Update(float timeDelta)
{
    UIObject::Late_Update(timeDelta);

}

HRESULT UI_MainTitleMenuButton::Render()
{
    if (!_isVisible) return S_OK;
    if (!_isSelected) return S_OK;

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", ETOI(EMainTitleTexture::TitleMenuBtn)), E_FAIL);

    // 하이라이트 색상 배율
    //float highlight = _isSelected ? 1.0f : 0.6f;
    //Vec4 color = { highlight, highlight, highlight, 1.f };
    //CHECK_FAILED(_shaderCom->Bind_RawValue("g_Color", &color, sizeof(Vec4)), E_FAIL);

    float alpha = _opacity;
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL); // UI 기본용 패스 확인 필요

    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

void UI_MainTitleMenuButton::Set_Selected(bool bSelected)
{
    _isSelected = bSelected;

    if (_labelUI)
    {
        _labelUI->Set_UITint(
            _isSelected
            ? Color(1.f, 1.f, 1.f, 1.f)
            : Color(0.85f, 0.85f, 0.85f, 1.f));
    }

    // UI Animation Tool에서 만들어서 진행
}

void UI_MainTitleMenuButton::Set_Visibility(bool active)
{
    UIObject::Set_Visibility(active);

    if (_labelUI)
        _labelUI->Set_Visibility(active);
}

HRESULT UI_MainTitleMenuButton::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_MAIN_TITLE, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

HRESULT UI_MainTitleMenuButton::Ready_ChildText(const FMainTitleMenuDesc* desc)
{
    UI_Text::FUITextDesc textDesc{};
    textDesc.name = format(L"{} Label", desc->name);

    // 부모 버튼 기준 로컬 오프셋 세팅
    textDesc.posX = desc->labelOffset.x;
    textDesc.posY = desc->labelOffset.y;

    textDesc.sizeX = (desc->labelSize.x > 0.f) ? desc->labelSize.x : desc->sizeX;
    textDesc.sizeY = (desc->labelSize.y > 0.f) ? desc->labelSize.y : desc->sizeY;

    textDesc.zOrder = _zOrder + 0.01f;
    textDesc.levelIndex = desc->levelIndex;

    textDesc.text = desc->labelText;
    textDesc.style.fontFamily = UI_DEFAULT_FONT_FAMILY;
    textDesc.style.fontSize = desc->fontSize;
    textDesc.style.color = Color(1.f, 1.f, 1.f, 1.f);
    textDesc.style.hAlign = ETextHAlign::Center;
    textDesc.style.vAlign = ETextVAlign::Middle;
    textDesc.style.wordWrap = false;

    auto childUI = static_pointer_cast<UI_Text>(
        GAME->Clone_UI(Protocol::OBJECT_TYPE_UI_TEXT, &textDesc));
    CHECK_NULL(childUI, E_FAIL);

    childUI->Set_LevelIndex(_levelIndex);
    childUI->Set_UILayer(Get_UILayer());

    // 버튼의 자식으로 붙인다.
    childUI->Get_Transform()->Set_Parent(this->Get_Transform());

    childUI->Set_Name(Get_Name() + L"." + textDesc.name);

    CHECK_FAILED(GAME->Register_UI(Get_UILayer(), childUI), E_FAIL);

    _labelUI = childUI;

    if (!_isVisible)
        _labelUI->Set_Visibility(false);

    return S_OK;
}


Shared<UI_MainTitleMenuButton> UI_MainTitleMenuButton::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_MainTitleMenuButton>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Prototype : UI_MainTitleMenuButton");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_MainTitleMenuButton::Clone(void* arg)
{
    auto clone = make_shared<UI_MainTitleMenuButton>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : UI_MainTitleMenuButton");
        return nullptr;
    }

    return clone;
}

void UI_MainTitleMenuButton::Free()
{
    UIObject::Free();
}

