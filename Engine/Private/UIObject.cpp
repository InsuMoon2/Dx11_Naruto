#include "pch.h"
#include "UIObject.h"

#include "Shader.h"

UIObject::UIObject(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

UIObject::UIObject(const UIObject& rhs)
    : GameObject(rhs)
    , _posX(rhs._posX), _posY(rhs._posY)
    , _sizeX(rhs._sizeX), _sizeY(rhs._sizeY)
    , _viewportWidth(rhs._viewportWidth), _viewportHeight(rhs._viewportHeight)
    , _worldMatrix(rhs._worldMatrix)
    , _forceVisibleInspector(rhs._forceVisibleInspector)
{
    memcpy(_transformMatrices, rhs._transformMatrices, sizeof(_transformMatrices));
}

UIObject::~UIObject()
{
}

HRESULT UIObject::Initialize_Prototype()
{
    GameObject::Initialize_Prototype();

    return S_OK;
}

HRESULT UIObject::Initialize(void* arg)
{
    CHECK_NULL(arg, E_FAIL);

    FUIDesc* desc = static_cast<FUIDesc*>(arg);

    // 부모의 Transform 세팅
    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    _posX = desc->posX;
    _posY = desc->posY;
    _sizeX = desc->sizeX;
    _sizeY = desc->sizeY;
    _zOrder = desc->zOrder;
    _levelIndex = desc->levelIndex;

    _viewportWidth = GAME->Get_UIViewportWidth();
    _viewportHeight = GAME->Get_UIViewportHeight();

    _transformCom->Set_LocalPosition(_posX, _posY, _zOrder);
    _transformCom->Set_LocalScale(_sizeX, _sizeY, 1.f);

    _transformMatrices[ETOI(ETransformState::View)] = Matrix::Identity;
    _transformMatrices[ETOI(ETransformState::Proj)] = XMMatrixOrthographicLH(
        _viewportWidth, _viewportHeight, 0.f, 1.f);

    // 초기 변환 행렬 계산
    Update_Transform();

    return S_OK;
}

void UIObject::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void UIObject::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    Vec3 localPos = _transformCom->Get_LocalPosition();

    if (_transformCom->Has_Parent())
    {
        auto parentUI = dynamic_pointer_cast<UIObject>(_transformCom->Get_Parent()->Get_Owner());

        if (parentUI)
        {
            _posX = parentUI->Get_UIPosX() + localPos.x;
            _posY = parentUI->Get_UIPosY() + localPos.y;
        }
        else
        {
            Vec3 parentLocalPos = _transformCom->Get_Parent()->Get_LocalPosition();
            _posX = parentLocalPos.x + localPos.x;
            _posY = parentLocalPos.y + localPos.y;
        }
    }

    else
    {
        _posX = localPos.x;
        _posY = localPos.y;
    }

    _sizeX = _transformCom->Get_LocalScale().x;
    _sizeY = _transformCom->Get_LocalScale().y;

    Update_Transform();
}

void UIObject::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);
}

HRESULT UIObject::Render()
{
    GameObject::Render();

    return S_OK;
}

void UIObject::Set_UIPosition(float x, float y)
{
    _posX = x;
    _posY = y;

    _transformCom->Set_LocalPosition(x, y, _zOrder);
    Update_Transform();
}

void UIObject::Set_UIScale(float x, float y)
{
    _sizeX = x;
    _sizeY = y;

    _transformCom->Set_LocalScale(x, y, 1.f);
    Update_Transform();
}

void UIObject::Set_UIRotationZ(float degree)
{
    _rotationZ = degree;

    Update_Transform();
}

void UIObject::Set_UIOpacity(float alpha)
{
    _opacity = clamp(alpha, 0.f, 1.f);
}

void UIObject::Set_UITint(const Color& color)
{
    _tintColor = color;
}

void UIObject::Update_Transform()
{
    const float viewW = GAME->Get_UIViewportWidth();
    const float viewH = GAME->Get_UIViewportHeight();

    const float uiScale = GAME->Get_UIScale();
    const Vec2 uiOffset = GAME->Get_UIViewportOffset();

    _transformMatrices[ETOI(ETransformState::Proj)] =
        XMMatrixOrthographicLH(viewW, viewH, 0.f, 1.f);

    const float finalSizeX = _sizeX * uiScale;
    const float finalSizeY = _sizeY * uiScale;

    // 기준 캔버스 좌표 -> 실제 viewport 중앙 영역
    const float finalPosX = uiOffset.x + (_posX * uiScale);
    const float finalPosY = uiOffset.y + (_posY * uiScale);

    const float ndcX = finalPosX - (viewW * 0.5f);
    const float ndcY = -finalPosY + (viewH * 0.5f);

    Matrix scaleMatrix = XMMatrixScaling(finalSizeX, finalSizeY, 1.f);
    Matrix rotMatrix = XMMatrixRotationZ(XMConvertToRadians(_rotationZ));
    Matrix transMatrix = XMMatrixTranslation(ndcX, ndcY, 0.f);

    _worldMatrix = scaleMatrix * rotMatrix * transMatrix;
}

HRESULT UIObject::Bind_ShaderResource(Shared<Shader> shader, const char* constantName, ETransformState transformState)
{
    CHECK_NULL(shader, E_FAIL);

    return shader->Bind_Matrix(constantName, &_transformMatrices[ETOI(transformState)]);
}

void UIObject::Free()
{
    GameObject::Free();
}
