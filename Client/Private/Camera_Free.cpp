#include "pch.h"
#include "Camera_Free.h"

IMPLEMENT_REFLECTION(Camera_Free)

bool Camera_Free::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "Camera_Free";

    PROPERTY_FLOAT("Mouse Sensor : ", _mouseSensor, 0.1f, 5.f);

    return true;
}

Camera_Free::Camera_Free(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Camera(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_CAMERA_FREE);
}

Camera_Free::Camera_Free(const Camera_Free& rhs)
    : Camera(rhs)
    , _mouseSensor(rhs._mouseSensor)
{
}

HRESULT Camera_Free::Initialize_Prototype()
{
    return Camera::Initialize_Prototype();
}

HRESULT Camera_Free::Initialize(void* arg)
{
    if (arg == nullptr)
        return E_FAIL;

    FCameraFreeDesc* desc = static_cast<FCameraFreeDesc*>(arg);
    _mouseSensor = desc->mouseSensor;

    CHECK_FAILED(Camera::Initialize(desc), E_FAIL);

    return S_OK;
}

void Camera_Free::BeginPlay()
{
    Camera::BeginPlay();

    //GAME->Set_ActiveCamera(GetSharedPtr<Camera>());
}

void Camera_Free::Priority_Update(float timeDelta)
{
    Camera::Priority_Update(timeDelta);

    if (!_inputEnabled)  
    {
        Update_TransformMatrices();
        return;
    }

    if (INPUT->KeyPress(KEY_TYPE::RBUTTON))
    {
        // 이동
        if (INPUT->KeyPress(KEY_TYPE::W)) _transformCom->Move_Backward(timeDelta);
        if (INPUT->KeyPress(KEY_TYPE::S)) _transformCom->Move_Forward(timeDelta);
        if (INPUT->KeyPress(KEY_TYPE::A)) _transformCom->Move_Left(timeDelta);
        if (INPUT->KeyPress(KEY_TYPE::D)) _transformCom->Move_Right(timeDelta);

        // 회전
        {
            Vec2 mouseDelta = INPUT->GetMouseDelta();

            if (mouseDelta.x != 0.f)
            {
                float angle = mouseDelta.x * _mouseSensor * timeDelta;
                _transformCom->Rotate_Axis(Vec3::Up, angle);
            }

            if (mouseDelta.y != 0.f)
            {
                float angle = mouseDelta.y * _mouseSensor * timeDelta;
                Vec3 right = _transformCom->Get_WorldRight();
                _transformCom->Rotate_Axis(right, angle);
            }
        }

    }

    // TODO : Zoom In, Zoom Out 구현 필요

    // 마지막에 Update 반드시 호출
    Update_TransformMatrices();
}

void Camera_Free::Update(float timeDelta)
{
    Camera::Update(timeDelta);
}

void Camera_Free::Late_Update(float timeDelta)
{
    Camera::Late_Update(timeDelta);
}

HRESULT Camera_Free::Render()
{
    return Camera::Render();
}

Shared<Camera_Free> Camera_Free::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Camera_Free>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : Camera_Free");
        instance->Free();
        return nullptr;
    }

    return instance;
}

Shared<GameObject> Camera_Free::Clone(void* arg)
{
    auto clone = make_shared<Camera_Free>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : Camera_Free");
        clone->Free();
        return nullptr;

    }
    return clone;
}

void Camera_Free::Free()
{
    Camera::Free();
}
