#include "pch.h"
#include "Background.h"

#include "GameObject_Factory.h"

#include "GameInstance.h"
#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"
#include "Level_MainTitle.h"

REGISTER_GAMEOBJECT(Background, Protocol::OBJECT_TYPE_BACKGROUND)

Background::Background(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject { device, context }
{
    
}

Background::Background(const Background& rhs)
    : UIObject{ rhs }
{
}

Background::~Background()
{
}

HRESULT Background::Initialize(void* arg)
{
    FBackgroundDesc* desc = static_cast<FBackgroundDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);

    _textureIndex = desc->textureIndex;
    _textureType = desc->textureType;
    _textDesc = desc->textDesc;

    CHECK_FAILED(UIObject::Initialize(desc), E_FAIL);

    CHECK_FAILED(Ready_Components(), E_FAIL);

    return S_OK;
}

HRESULT Background::Initialize_Prototype()
{
    return UIObject::Initialize_Prototype();
}

void Background::Priority_Update(float timeDelta)
{
    UIObject::Priority_Update(timeDelta);
}

void Background::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    //__super::Update_Transform();
}

void Background::Late_Update(float timeDelta)
{
    UIObject::Late_Update(timeDelta);

}

HRESULT Background::Render()
{
    if (!_isVisible)
        return S_OK;

    UIObject::Render();

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix), E_FAIL);

    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", _textureIndex), E_FAIL);

    float fAlpha = 1.f;
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &fAlpha, sizeof(float)), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

void Background::Set_Visibility(bool active)
{
    UIObject::Set_Visibility(active);

    if (_labelUI)
        _labelUI->Set_Visibility(active);
}

HRESULT Background::Setup_OptionalText(EUILayer uiLayer)
{
    if (_labelUI)
        return S_OK;

    if (_textDesc.text.empty())
        return S_OK;

    return Ready_ChildText(uiLayer);
}

HRESULT Background::On_UIRegistered()
{
    return Setup_OptionalText(Get_UILayer());
}

void Background::Set_LabelText(const wstring& text)
{
    _textDesc.text = text;

    if (_labelUI)
        _labelUI->Set_Text(text);
}

HRESULT Background::Ready_Components()
{
    CHECK_FAILED(Add_Component(_textureType, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

HRESULT Background::Ready_ChildText(EUILayer uiLayer)
{
    UI_Text::FUITextDesc textDesc{};
    textDesc.name = format(L"{} Label", Get_Name());

    textDesc.posX = _textDesc.offset.x;
    textDesc.posY = _textDesc.offset.y;
    textDesc.sizeX = (_textDesc.size.x > 0.f) ? _textDesc.size.x : _sizeX;
    textDesc.sizeY = (_textDesc.size.y > 0.f) ? _textDesc.size.y : _sizeY;
    textDesc.zOrder = _zOrder + _textDesc.zOrderOffset;
    textDesc.levelIndex = _levelIndex;
    textDesc.text = _textDesc.text;
    textDesc.style = _textDesc.style;

    _labelUI = Create_ChildText(uiLayer, textDesc);
    CHECK_NULL(_labelUI, E_FAIL);

    if (!_isVisible)
        _labelUI->Set_Visibility(false);

    return S_OK;
}

Shared<UI_Text> Background::Create_ChildText(EUILayer uiLayer, UI_Text::FUITextDesc& textDesc)
{
    UI_Text::FUITextDesc cloneDesc = textDesc;

    auto childUI = static_pointer_cast<UI_Text>(
        GAME->Clone_UI(Protocol::OBJECT_TYPE_UI_TEXT, &cloneDesc));

    if (!childUI)
        return nullptr;

    childUI->Set_LevelIndex(_levelIndex);
    childUI->Set_UILayer(uiLayer);

    childUI->Get_Transform()->Set_Parent(this->Get_Transform());

    childUI->Set_Name(Get_Name() + L"." + textDesc.name);

    if (FAILED(GAME->Register_UI(uiLayer, childUI)))
        return nullptr;

    return childUI;
}

Shared<UIObject> Background::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Background>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Prototype : Background");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Background::Clone(void* arg)
{
    auto clone = make_shared<Background>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : Background");

        return nullptr;
    }

    return clone;
}

void Background::Free()
{
    UIObject::Free();
}
