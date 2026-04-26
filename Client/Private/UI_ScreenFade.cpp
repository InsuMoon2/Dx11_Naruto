#include "pch.h"
#include "UI_ScreenFade.h"

#include "GameObject_Factory.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"

REGISTER_GAMEOBJECT(UI_ScreenFade, Protocol::OBJECT_TYPE_UI_SCREEN_FADE)
IMPLEMENT_REFLECTION(UI_ScreenFade)

bool UI_ScreenFade::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "UI_ScreenFade";

    PROPERTY_UIOBJECT_FORCE_VISIBLE();

    return true;
}

static constexpr uint32 SCREEN_FADE_SOLID_COLOR_PASS = 8;

UI_ScreenFade::UI_ScreenFade(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
}

UI_ScreenFade::UI_ScreenFade(const UI_ScreenFade& rhs)
    : UIObject(rhs)
    , _fadeColor(rhs._fadeColor)
    , _fadeAlpha(rhs._fadeAlpha)
{
}

HRESULT UI_ScreenFade::Initialize_Prototype()
{
    return UIObject::Initialize_Prototype();
}

HRESULT UI_ScreenFade::Initialize(void* arg)
{
    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);

    if (arg)
    {
        auto* desc = static_cast<FScreenFadeDesc*>(arg);
        _fadeColor = desc->fadeColor;
        _fadeAlpha = clamp(desc->initialAlpha, 0.f, 1.f);
    }

    CHECK_FAILED(Ready_Components(), E_FAIL);

    Set_Visibility(_fadeAlpha > 0.f);

    return S_OK;
}

void UI_ScreenFade::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    __super::Update_Transform();
}

HRESULT UI_ScreenFade::Render()
{
    if (!Is_VisibleForRender())
        return S_OK;

    CHECK_NULL(_shaderCom, E_FAIL);
    CHECK_NULL(_bufferCom, E_FAIL);

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColor", &_fadeColor, sizeof(Color)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &_fadeAlpha, sizeof(float)), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin_Pass(SCREEN_FADE_SOLID_COLOR_PASS), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

void UI_ScreenFade::Set_FadeAlpha(float alpha)
{
    _fadeAlpha = clamp(alpha, 0.f, 1.f);
    Set_Visibility(_fadeAlpha > 0.f);
}

void UI_ScreenFade::Set_FadeColor(const Color& color)
{
    _fadeColor = color;
}

HRESULT UI_ScreenFade::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

Shared<UI_ScreenFade> UI_ScreenFade::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_ScreenFade>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : UI_ScreenFade");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_ScreenFade::Clone(void* arg)
{
    auto clone = make_shared<UI_ScreenFade>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : UI_ScreenFade");
        return nullptr;
    }

    return clone;
}

void UI_ScreenFade::Free()
{
    UIObject::Free();
}
