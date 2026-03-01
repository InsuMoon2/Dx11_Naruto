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

    _viewportWidth = GAME->Get_ViewportWidth();
    _viewportHeight = GAME->Get_ViewportHeight();

    _transformCom->Set_LocalPosition(_posX, _posY, 0.f);
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

    _posX = _transformCom->Get_LocalPosition().x;
    _posY = _transformCom->Get_LocalPosition().y;
    _sizeX = _transformCom->Get_LocalScale().x;
    _sizeY = _transformCom->Get_LocalScale().y;
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

void UIObject::Update_Transform()
{
    float designX = GAME->Get_WindowWidth();
    float designY = GAME->Get_WindowHeight();

    float currentViewX = GAME->Get_ViewportWidth();
    float currentViewY = GAME->Get_ViewportHeight();

    // 현재 크기 / 원본 크기
    float ratioX = currentViewX / designX;
    float ratioY = currentViewY / designY;

    float finalSizeX = _sizeX * ratioX;
    float finalSizeY = _sizeY * ratioY;
    float finalPosX = _posX * ratioX;
    float finalPosY = _posY * ratioY;

    // UI 크기 (픽셀 단위로)
    Matrix scaleMatrix = XMMatrixScaling(finalSizeX, finalSizeY, 1.f);

    // Translatino : 화면 좌표 -> NDC 좌표 변환
    float ndcX = finalPosX - (currentViewX * 0.5f);
    float ndcY = -finalPosY + (currentViewY * 0.5f);

    Matrix transMatrix = XMMatrixTranslation(ndcX, ndcY, _zOrder);

    _worldMatrix = scaleMatrix * transMatrix;
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
