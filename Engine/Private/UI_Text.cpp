#include "pch.h"
#include "UI_Text.h"

UI_Text::UI_Text(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_UI_TEXT);
}

UI_Text::UI_Text(const UI_Text& rhs)
    : UIObject(rhs)
    , _text(rhs._text)
    , _style(rhs._style)
{
}

HRESULT UI_Text::Initialize_Prototype()
{
    return UIObject::Initialize_Prototype();
}

HRESULT UI_Text::Initialize(void* arg)
{
    auto* desc = static_cast<FUITextDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);

    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);

    _text = desc->text;
    _style = desc->style;

    return S_OK;
}

void UI_Text::Priority_Update(float timeDelta)
{
    UIObject::Priority_Update(timeDelta);
}

void UI_Text::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    Update_Transform();
}

void UI_Text::Late_Update(float timeDelta)
{
    UIObject::Late_Update(timeDelta);
}

HRESULT UI_Text::Render()
{
    if (!_isVisible || _text.empty())
        return S_OK;

    RECT rect = Build_ScreenRect();

    FTextStyle finalStyle = _style;
    finalStyle.color.x *= _tintColor.x;
    finalStyle.color.y *= _tintColor.y;
    finalStyle.color.z *= _tintColor.z;
    finalStyle.color.w *= _tintColor.w;
    finalStyle.color.w *= _opacity;

    HRESULT hr = GAME->Draw_Text(_text, rect, finalStyle);
    CHECK_FAILED(hr, E_FAIL);

    return S_OK;
}

RECT UI_Text::Build_ScreenRect() const
{
    float designX = GAME->Get_WindowWidth();
    float designY = GAME->Get_WindowHeight();

    float currentViewX = GAME->Get_UIViewportWidth();
    float currentViewY = GAME->Get_UIViewportHeight();

    float ratioX = currentViewX / designX;
    float ratioY = currentViewY / designY;

    float halfW = _sizeX * 0.5f;
    float halfH = _sizeY * 0.5f;

    float left = (_posX - halfW) * ratioX;
    float top = (_posY - halfH) * ratioY;
    float right = (_posX + halfW) * ratioX;
    float bottom = (_posY + halfH) * ratioY;

    RECT rc = {};
    rc.left = static_cast<LONG>(left);
    rc.top = static_cast<LONG>(top);
    rc.right = static_cast<LONG>(right);
    rc.bottom = static_cast<LONG>(bottom);

    return rc;
}

Shared<UI_Text> UI_Text::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_Text>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Prototype : UI_Text");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_Text::Clone(void* arg)
{
    auto instance = make_shared<UI_Text>(*this);

    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : UI_Text");
        return nullptr;
    }

    return instance;
}

void UI_Text::Free()
{
    UIObject::Free();
}
