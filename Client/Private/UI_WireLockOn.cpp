#include "pch.h"
#include "UI_WireLockOn.h"
#include "Shader.h"
#include "Texture.h"
#include "VIBuffer_Rect.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(UI_WireLockOn, Protocol::OBJECT_TYPE_UI_LOCK_ON)

UI_WireLockOn::UI_WireLockOn(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
}

UI_WireLockOn::UI_WireLockOn(const UI_WireLockOn& rhs)
    : UIObject(rhs)
    , _bgSize(rhs._bgSize)
    , _lockSize(rhs._lockSize)
    , _animTime(rhs._animTime)
    , _innerAlpha(rhs._innerAlpha)
{
}

HRESULT UI_WireLockOn::Initialize_Prototype()
{
    return UIObject::Initialize_Prototype();
}

HRESULT UI_WireLockOn::Initialize(void* arg)
{
    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FWireLockOnDesc*>(arg);
    if (desc)
    {
        _bgSize = desc->bgSize;
        _lockSize = desc->lockSize;
    }

    CHECK_FAILED(Ready_Components(), E_FAIL);

    Set_Visibility(false);

    return S_OK;
}

void UI_WireLockOn::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    if (!_isVisible)
        return;

    _animTime += timeDelta;

    const float pulse = 0.92f + (sinf(_animTime * 10.f) * 0.08f);
    _innerAlpha = 0.75f + (sinf(_animTime * 12.f) * 0.25f);

    Set_UIScale(_bgSize, _bgSize);

    __super::Update_Transform();
}

HRESULT UI_WireLockOn::Render()
{
    if (!_isVisible)
        return S_OK;

    CHECK_NULL(_shaderCom, E_FAIL);
    CHECK_NULL(_textureCom, E_FAIL);
    CHECK_NULL(_bufferCom, E_FAIL);

    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    const Vec3 center = _worldMatrix.Translation();

    const Matrix bgMatrix =
        Matrix::CreateScale(_bgSize, _bgSize, 1.f) *
        Matrix::CreateTranslation(center.x, center.y, center.z);

    const float pulse = 0.92f + (sinf(_animTime * 10.f) * 0.08f);
    const float innerRotation = _animTime * 180.f;

    const Matrix innerMatrix =
        Matrix::CreateScale(_lockSize * pulse, _lockSize * pulse, 1.f) *
        Matrix::CreateRotationZ(XMConvertToRadians(innerRotation)) *
        Matrix::CreateTranslation(center.x, center.y, center.z + 0.001f);

    CHECK_FAILED(Render_Texture(LOCK_BG_TEXTURE_INDEX, bgMatrix, 1.f), E_FAIL);
    CHECK_FAILED(Render_Texture(LOCK_INNER_TEXTURE_INDEX, innerMatrix, _innerAlpha), E_FAIL);

    return S_OK;
}

void UI_WireLockOn::Show_LockOn(float x, float y)
{
    _animTime = 0.f;

    Set_UIPosition(x, y);
    Set_Visibility(true);
}

void UI_WireLockOn::Hide_LockOn()
{
    Set_Visibility(false);
}

HRESULT UI_WireLockOn::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_LOCK_ON, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

HRESULT UI_WireLockOn::Render_Texture(uint32 textureIndex, const Matrix& worldMatrix, float alpha)
{
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &worldMatrix), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);
    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", textureIndex), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin_Pass(7), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

Shared<UI_WireLockOn> UI_WireLockOn::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_WireLockOn>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : UI_WireLockOn");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_WireLockOn::Clone(void* arg)
{
    auto clone = make_shared<UI_WireLockOn>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : UI_WireLockOn");
        return nullptr;
    }

    return clone;
}

void UI_WireLockOn::Free()
{
    UIObject::Free();
}
