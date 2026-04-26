#include "pch.h"
#include "UI_MissionMarker.h"

#include "GameInstance.h"
#include "GameObject_Factory.h"
#include "Shader.h"
#include "Texture.h"
#include "Transform.h"
#include "VIBuffer_Rect.h"

REGISTER_GAMEOBJECT(UI_MissionMarker, Protocol::OBJECT_TYPE_UI_MISSION_MARKER)

NS_BEGIN(Client)

IMPLEMENT_REFLECTION(UI_MissionMarker)

bool UI_MissionMarker::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "UI_MissionMarker";

    PROPERTY_UIOBJECT_FORCE_VISIBLE();

    return true;
}

UI_MissionMarker::UI_MissionMarker(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
}

UI_MissionMarker::UI_MissionMarker(const UI_MissionMarker& rhs)
    : UIObject(rhs)
    , _screenOffsetY(rhs._screenOffsetY)
    , _edgeMargin(rhs._edgeMargin)
    , _followSpeed(rhs._followSpeed)
    , _insideScale(rhs._insideScale)
    , _edgeScale(rhs._edgeScale)
    , _textureIndex(rhs._textureIndex)
{
}

HRESULT UI_MissionMarker::Initialize_Prototype()
{
    return UIObject::Initialize_Prototype();
}

HRESULT UI_MissionMarker::Initialize(void* arg)
{
    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);
    CHECK_FAILED(Ready_Components(), E_FAIL);

    if (arg)
    {
        auto* desc = static_cast<FUIMissionMarkerDesc*>(arg);
        _textureIndex = desc->textureIndex;
    }
    Set_UIScale(_insideScale, _insideScale);
    Set_Visibility(false);

    return S_OK;
}

void UI_MissionMarker::Set_TargetObject(Weak<GameObject> targetObject)
{
    _targetObject = targetObject;
    _hasFixedWorldPosition = false;
    _hasCurrentUIPosition = false;

    Set_Visibility(!_targetObject.expired());
}

void UI_MissionMarker::Set_TargetWorldPosition(const Vec3& worldPosition)
{
    _targetObject.reset();
    _targetWorldPosition = worldPosition;
    _hasFixedWorldPosition = true;
    _hasCurrentUIPosition = false;
    Set_Visibility(true);
}

void UI_MissionMarker::Clear_Target()
{
    _targetObject.reset();
    _hasFixedWorldPosition = false;
    _hasCurrentUIPosition = false;
    Set_Visibility(false);
}

void UI_MissionMarker::Update(float timeDelta)
{
    UIObject::Update(timeDelta);

    _ambientHaloTime += timeDelta;

    Vec3 targetWorldPosition = Vec3::Zero;
    if (!Resolve_TargetWorldPosition(targetWorldPosition))
    {
        Set_Visibility(false);
        return;
    }

    Vec2 targetUIPosition = Vec2::Zero;
    bool isInsideScreen = false;
    if (!Project_WorldToUI(targetWorldPosition, targetUIPosition, isInsideScreen))
    {
        Set_Visibility(false);
        return;
    }

    if (isInsideScreen)
    {
        targetUIPosition.y -= _screenOffsetY;
    }
    else
    {
        targetUIPosition = Clamp_ToScreenEdge(targetUIPosition);
    }

    if (isInsideScreen)
    {
        _currentUIPosition = targetUIPosition;
        _hasCurrentUIPosition = true;
    }
    else if (!_hasCurrentUIPosition)
    {
        _currentUIPosition = targetUIPosition;
        _hasCurrentUIPosition = true;
    }
    else
    {
        _currentUIPosition = Smooth_Follow(_currentUIPosition, targetUIPosition, timeDelta);
    }

    Set_Visibility(true);
    Set_UIPosition(_currentUIPosition.x, _currentUIPosition.y);

    const float targetScale = isInsideScreen ? _insideScale : _edgeScale;
    Set_UIScale(targetScale, targetScale);

    __super::Update_Transform();
}

void UI_MissionMarker::Late_Update(float timeDelta)
{
    if (!Is_VisibleForRender())
        return;

    UIObject::Late_Update(timeDelta);
}

HRESULT UI_MissionMarker::Render()
{
    if (!Is_VisibleForRender())
        return S_OK;

    CHECK_FAILED(Render_AmbientHalo(), E_FAIL);

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", _textureIndex), E_FAIL);

    Vec4 iconColor = Vec4(1.f, 1.f, 1.f, 1.f);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColor", &iconColor, sizeof(Vec4)), E_FAIL);

    float alpha = 1.f;
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

HRESULT UI_MissionMarker::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_MISSION_MARKER, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_HALO, _haloTextureCom), E_FAIL);

    return S_OK;
}

bool UI_MissionMarker::Resolve_TargetWorldPosition(Vec3& outWorldPosition) const
{
    if (!_targetObject.expired())
    {
        auto targetObject = _targetObject.lock();
        if (!targetObject || targetObject->Is_Destroy())
            return false;

        auto transform = targetObject->Get_Transform();
        if (!transform)
            return false;

        outWorldPosition = transform->Get_WorldPosition();
        return true;
    }

    if (_hasFixedWorldPosition)
    {
        outWorldPosition = _targetWorldPosition;
        return true;
    }

    return false;
}

bool UI_MissionMarker::Project_WorldToUI(const Vec3& worldPosition, Vec2& outUIPosition, bool& outIsInsideScreen) const
{
    const float uiScale = GAME->Get_UIScale();
    if (uiScale <= FLT_EPSILON)
        return false;

    const Matrix viewMatrix = *GAME->Get_Transform(ETransformState::View);
    const Matrix projMatrix = *GAME->Get_Transform(ETransformState::Proj);

    Vec4 clipSpace = XMVector4Transform(
        Vec4(worldPosition.x, worldPosition.y, worldPosition.z, 1.f),
        viewMatrix * projMatrix);

    if (fabsf(clipSpace.w) <= FLT_EPSILON)
        return false;

    const bool isBehindCamera = clipSpace.w <= 0.f;
    const float viewportWidth = GAME->Get_UIViewportWidth();
    const float viewportHeight = GAME->Get_UIViewportHeight();
    const Vec2 uiOffset = GAME->Get_UIViewportOffset();
    const float referenceWidth = GAME->Get_UIReferenceWidth();
    const float referenceHeight = GAME->Get_UIReferenceHeight();

    if (isBehindCamera)
    {
        Vec4 viewSpace = XMVector4Transform(
            Vec4(worldPosition.x, worldPosition.y, worldPosition.z, 1.f),
            viewMatrix);

        const float horizontal = (fabsf(viewSpace.x) <= FLT_EPSILON)
            ? 0.f
            : viewSpace.x;

        outUIPosition = Vec2(
            referenceWidth * 0.5f + horizontal,
            referenceHeight);
        outIsInsideScreen = false;
        return true;
    }

    const float invW = 1.f / clipSpace.w;

    float ndcX = clipSpace.x * invW;
    float ndcY = clipSpace.y * invW;

    Vec2 screenPosition;
    screenPosition.x = (ndcX * 0.5f + 0.5f) * viewportWidth;
    screenPosition.y = (-ndcY * 0.5f + 0.5f) * viewportHeight;

    outUIPosition.x = (screenPosition.x - uiOffset.x) / uiScale;
    outUIPosition.y = (screenPosition.y - uiOffset.y) / uiScale;

    outIsInsideScreen =
        ndcX >= -1.f && ndcX <= 1.f &&
        ndcY >= -1.f && ndcY <= 1.f &&
        outUIPosition.x >= 0.f && outUIPosition.x <= referenceWidth &&
        outUIPosition.y >= 0.f && outUIPosition.y <= referenceHeight;

    return true;
}

Vec2 UI_MissionMarker::Clamp_ToScreenEdge(const Vec2& uiPosition) const
{
    const float width = GAME->Get_UIReferenceWidth();
    const float height = GAME->Get_UIReferenceHeight();

    const Vec2 center(width * 0.5f, height * 0.5f);
    Vec2 direction = uiPosition - center;

    if (direction.LengthSquared() <= FLT_EPSILON)
        direction = Vec2(0.f, -1.f);

    const float halfWidth = max(1.f, width * 0.5f - _edgeMargin);
    const float halfHeight = max(1.f, height * 0.5f - _edgeMargin);

    const float scaleX = (fabsf(direction.x) <= FLT_EPSILON) ? FLT_MAX : halfWidth / fabsf(direction.x);
    const float scaleY = (fabsf(direction.y) <= FLT_EPSILON) ? FLT_MAX : halfHeight / fabsf(direction.y);
    const float edgeScale = min(scaleX, scaleY);

    Vec2 clamped = center + direction * edgeScale;

    clamped.x = ::clamp(clamped.x, _edgeMargin, width - _edgeMargin);
    clamped.y = ::clamp(clamped.y, _edgeMargin, height - _edgeMargin);

    return clamped;
}

Vec2 UI_MissionMarker::Smooth_Follow(const Vec2& currentPosition, const Vec2& targetPosition, float timeDelta) const
{
    const float alpha = 1.f - expf(-_followSpeed * max(0.f, timeDelta));
    return Vec2::Lerp(currentPosition, targetPosition, ::clamp(alpha, 0.f, 1.f));
}

HRESULT UI_MissionMarker::Render_AmbientHalo()
{
    CHECK_NULL(_haloTextureCom, E_FAIL);

    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    const float phase = fmodf(_ambientHaloTime / _ambientHaloCycle, 1.f);

    const float alpha = (1.f - phase) * _ambientHaloBaseAlpha;
    const float scale = ::lerp(_ambientHaloStartScale, _ambientHaloEndScale, phase);

    Matrix haloWorldMatrix = Matrix::CreateScale(scale, scale, 1.f) * _worldMatrix;

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &haloWorldMatrix), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Alpha", &alpha, sizeof(float)), E_FAIL);
    CHECK_FAILED(_haloTextureCom->Bind_SRV(_shaderCom, "g_Texture", 0), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

Shared<UI_MissionMarker> UI_MissionMarker::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_MissionMarker>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : UI_MissionMarker");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_MissionMarker::Clone(void* arg)
{
    auto clone = make_shared<UI_MissionMarker>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : UI_MissionMarker");
        return nullptr;
    }

    return clone;
}

void UI_MissionMarker::Free()
{
    _targetObject.reset();
    UIObject::Free();
}

NS_END
