#include "pch.h"
#include "UI_LoadingProgressBar.h"
#include "Loader.h"
#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"

UI_LoadingProgressBar::UI_LoadingProgressBar(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_UI_LOADING_PROGRESS_BAR);
}

UI_LoadingProgressBar::UI_LoadingProgressBar(const UI_LoadingProgressBar& rhs)
    : UIObject(rhs)
    , _ratio(rhs._ratio)
    , _textureIndex(rhs._textureIndex)
    , _textureType(rhs._textureType)
{
}

UI_LoadingProgressBar::~UI_LoadingProgressBar()
{
}

HRESULT UI_LoadingProgressBar::Initialize(void* arg)
{
    auto* desc = static_cast<FLoadingProgressBarDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);

    _textureIndex = desc->textureIndex;
    _textureType = desc->textureType;

    CHECK_FAILED(UIObject::Initialize(desc), E_FAIL);
    CHECK_FAILED(Ready_Components(), E_FAIL);

    return S_OK;
}

HRESULT UI_LoadingProgressBar::Initialize_Prototype()
{
    return UIObject::Initialize_Prototype();
}

void UI_LoadingProgressBar::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    __super::Update_Transform();

    auto loader = _loader.lock();
    if (!loader)
        return;

    _ratio = ::clamp(loader->Get_ProgressRatio(), 0.f, 1.f);
}

HRESULT UI_LoadingProgressBar::Render()
{
    if (!_isVisible)
        return S_OK;

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);
    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", _textureIndex), E_FAIL);

    float startU = 0.f;
    float endU = 1.f;
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FillStartU", &startU, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_FillEndU", &endU, sizeof(float)), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);

    // Background
    {
        Vec4 color = { 0.f, 0.f, 0.f, 1.f };
        float ratio = 1.f;
        float alpha = 0.75f;

        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColor", &color, sizeof(Vec4)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_FillRatio", &ratio, sizeof(float)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);

        CHECK_FAILED(_shaderCom->Begin_Pass(3), E_FAIL);
        CHECK_FAILED(_bufferCom->Render(), E_FAIL);
    }

    // Fill
    {
        Vec4 color = { 1.f, 1.f, 1.f, 1.f };
        float alpha = 1.f;

        CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColor", &color, sizeof(Vec4)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_FillRatio", &_ratio, sizeof(float)), E_FAIL);

        CHECK_FAILED(_shaderCom->Begin_Pass(3), E_FAIL);
        CHECK_FAILED(_bufferCom->Render(), E_FAIL);
    }
   

    return S_OK;
}

HRESULT UI_LoadingProgressBar::Ready_Components()
{
    CHECK_FAILED(Add_Component(_textureType, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

Shared<UIObject> UI_LoadingProgressBar::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_LoadingProgressBar>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Prototype : UI_LoadingProgressBar");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_LoadingProgressBar::Clone(void* arg)
{
    auto clone = make_shared<UI_LoadingProgressBar>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : UI_LoadingProgressBar");

        return nullptr;
    }

    return clone;
}

void UI_LoadingProgressBar::Free()
{
    UIObject::Free();
}
