#include "pch.h"
#include "UI_Text.h"

UIText::UIText(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
}

UIText::UIText(const UIText& rhs)
    : UIObject(rhs)
    , _text(rhs._text)
    , _style(rhs._style)
{
}

HRESULT UIText::Initialize_Prototype()
{
    return UIObject::Initialize_Prototype();
}

HRESULT UIText::Initialize(void* arg)
{
    auto* desc = static_cast<FUITextDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);

    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);

    _text = desc->text;
    _style = desc->style;

    return S_OK;
}

void UIText::Priority_Update(float timeDelta)
{
    UIObject::Priority_Update(timeDelta);
}

void UIText::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    Update_Transform();
}

void UIText::Late_Update(float timeDelta)
{
    UIObject::Late_Update(timeDelta);
}

HRESULT UIText::Render()
{
    if (!_isVisible)
        return S_OK;

    if (_text.empty())
        return S_OK;

    RECT rect = Build_ScreenRect();
    CHECK_FAILED(GAME->Draw_Text(_text, rect, _style), E_FAIL);

    return S_OK;
}

RECT UIText::Build_ScreenRect() const
{
    float designX = GAME->Get_WindowWidth();
    float designY = GAME->Get_WindowHeight();

    float currentViewX = GAME->Get_ViewportWidth();
    float currentViewY = GAME->Get_ViewportHeight();

    float ratioX = currentViewX / designX;
    float ratioY = currentViewY / designY;

    float left = _posX * ratioX;
    float top = _posY * ratioY;
    float right = (_posX + _sizeX) * ratioX;
    float bottom = (_posY + _sizeY) * ratioY;

    RECT rc = {};
    rc.left = static_cast<LONG>(left);
    rc.top = static_cast<LONG>(top);
    rc.right = static_cast<LONG>(right);
    rc.bottom = static_cast<LONG>(bottom);

    return rc;
}

Shared<UIText> UIText::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, void* arg)
{
    auto instance = make_shared<UIText>(device, context);

    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Failed to Create : UIText");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UIText::Clone(void* arg)
{
    auto instance = make_shared<UIText>(*this);

    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : UIText");
        return nullptr;
    }

    return instance;
}

void UIText::Free()
{
    UIObject::Free();
}
