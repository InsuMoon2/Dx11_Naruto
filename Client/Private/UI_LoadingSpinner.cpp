#include "pch.h"
#include "UI_LoadingSpinner.h"
#include "GameInstance.h"
#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(UI_LoadingSpinner, Protocol::OBJECT_TYPE_UI_LOADING_SPINNER)

UI_LoadingSpinner::UI_LoadingSpinner(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_UI_LOADING_SPINNER);
}

UI_LoadingSpinner::UI_LoadingSpinner(const UI_LoadingSpinner& rhs)
    : UIObject(rhs)
    , _textureIndex(rhs._textureIndex)
    , _textureType(rhs._textureType)
    , _rotationSpeed(rhs._rotationSpeed)
    , _angle(rhs._angle)
{
}

HRESULT UI_LoadingSpinner::Initialize(void* arg)
{
    auto* desc = static_cast<FLoadingSpinnerDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);

    _textureIndex = desc->textureIndex;
    _textureType = desc->textureType;
    _rotationSpeed = desc->rotationSpeed;

    CHECK_FAILED(UIObject::Initialize(desc), E_FAIL);
    CHECK_FAILED(Ready_Components(), E_FAIL);

    return S_OK;
}

HRESULT UI_LoadingSpinner::Initialize_Prototype()
{
    return UIObject::Initialize_Prototype();
}

void UI_LoadingSpinner::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    _angle += _rotationSpeed * timeDelta;

    if (_angle > XM_2PI)
        _angle -= XM_2PI;

    __super::Update_Transform();
}

HRESULT UI_LoadingSpinner::Render()
{
    if (!_isVisible)
        return S_OK;

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", _textureIndex), E_FAIL);

    float alpha = 1.f;
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_RotationAngle", &_angle, sizeof(float)), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin_Pass(4), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

HRESULT UI_LoadingSpinner::Ready_Components()
{
    CHECK_FAILED(Add_Component(_textureType, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

Shared<UIObject> UI_LoadingSpinner::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_LoadingSpinner>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Prototype : UI_LoadingSpinner");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_LoadingSpinner::Clone(void* arg)
{
    auto clone = make_shared<UI_LoadingSpinner>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : UI_LoadingSpinner");

        return nullptr;
    }

    return clone;
}

void UI_LoadingSpinner::Free()
{
    UIObject::Free();
}
