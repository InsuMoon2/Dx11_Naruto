#include "pch.h"
#include "Camera.h"
#include "Input_Manager.h"

Camera::Camera(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

Camera::Camera(const Camera& rhs)
    : GameObject(rhs)
    , _fovY(rhs._fovY)
    , _nearZ(rhs._nearZ)
    , _farZ(rhs._farZ)
    , _aspect(rhs._aspect)
{
}

HRESULT Camera::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT Camera::Initialize(void* arg)
{
    if (arg == nullptr)
        return S_OK;

    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    FCameraDesc* desc = static_cast<FCameraDesc*>(arg);

    // Transform에 카메라 위치 세팅
    _transformCom->Set_LocalPosition(desc->eye);
    _transformCom->LookAt(desc->at);

    // 뷰포트 화면비 세팅
    uint32 numViewports = 1; // TODO : 지금은 1이지만, 멀티플레이 시 변경해야함
    D3D11_VIEWPORT viewport = {};

    _context->RSGetViewports(&numViewports, &viewport);

    _fovY = desc->fovY;
    _nearZ = desc->nearZ;
    _farZ = desc->farZ;
    _aspect = viewport.Width / viewport.Height;

    Update_TransformMatrices();

    return S_OK;
}

void Camera::BeginPlay()
{
    GameObject::BeginPlay();

    GAME->Register_Camera(static_pointer_cast<Camera>(GetSharedPtr()));
}

void Camera::Priority_Update(float timeDelta)
{
    if (!GAME->Is_ActiveCamera(GetSharedPtr<Camera>()))
        return;

    GameObject::Priority_Update(timeDelta);

    // 클라이언트 카메라에서 카메라 이동/회전 후 호출
}

void Camera::Update(float timeDelta)
{
    GameObject::Update(timeDelta);
}

void Camera::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);
}

HRESULT Camera::Render()
{
    return GameObject::Render();
}

void Camera::Update_TransformMatrices()
{
    if(GAME->Is_ActiveCamera(static_pointer_cast<Camera>(GetSharedPtr())) == false)
        return;

    /* View */
    {
        Matrix worldMatrix = _transformCom->Get_WorldMatrix();
        Matrix viewMatrix = worldMatrix.Invert();
        GAME->Set_Transform(ETransformState::View, viewMatrix);
    }

    /* Proj */
    {
        Matrix projMatrix = XMMatrixPerspectiveFovLH(_fovY, _aspect, _nearZ, _farZ);
        GAME->Set_Transform(ETransformState::Proj, projMatrix);
    }
}

void Camera::Free()
{
    GameObject::Free();
}
