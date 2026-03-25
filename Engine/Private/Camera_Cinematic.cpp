#include "pch.h"
#include "Camera_Cinematic.h"
#include "GameObject_Factory.h"
#include "Input_Manager.h"

REGISTER_GAMEOBJECT(Camera_Cinematic, Protocol::OBJECT_TYPE_CAMERA_CINEMATIC)

IMPLEMENT_REFLECTION(Camera_Cinematic)

bool Camera_Cinematic::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "Camera_Cinematic";

    PROPERTY_FLOAT("Mouse Sensor : ", _mouseSensor, 4.f, 5.f);
    PROPERTY_FLOAT("Camera Speed : ", _cameraSpeed, 1.f, 100.f);
    PROPERTY_FLOAT("Distance : ", _distance, 1.f, 50.f);

    return true;
}

Camera_Cinematic::Camera_Cinematic(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Camera(device, context)
{
}

Camera_Cinematic::Camera_Cinematic(const Camera_Cinematic& rhs)
    : Camera(rhs)
    , _mode(rhs._mode)
    , _mouseSensor(rhs._mouseSensor)
    , _cameraSpeed(rhs._cameraSpeed)
    , _distance(rhs._distance)
    , _targetOffset(rhs._targetOffset)
{

}

HRESULT Camera_Cinematic::Initialize_Prototype()
{
    return Camera::Initialize_Prototype();
}

HRESULT Camera_Cinematic::Initialize(void* arg)
{
    if (!arg)
        return E_FAIL;

    auto* desc = static_cast<FCinematicDesc*>(arg);
    _mode = desc->mode;

    _mouseSensor = desc->mouseSensor;
    _cameraSpeed = desc->cameraSpeed;

    CHECK_FAILED(Camera::Initialize(arg), E_FAIL);

    return S_OK;
}

void Camera_Cinematic::BeginPlay()
{
    Camera::BeginPlay();

}

void Camera_Cinematic::Priority_Update(float timeDelta)
{
    Camera::Priority_Update(timeDelta);

    if (!_inputEnabled)
    {
        Update_TransformMatrices();
        return;
    }

    switch (_mode)
    {
    case ECineCameraMode::Free:    Update_FreeMode(timeDelta);    break;
    case ECineCameraMode::Target:  Update_TargetMode(timeDelta);  break;
    case ECineCameraMode::LookAt:  Update_LookAtMode(timeDelta);  break;
    case ECineCameraMode::Rail:    /* Rail은 보간으로만 동작 */    break;
    default: break;
    }

    Update_TransformMatrices();
}

void Camera_Cinematic::Update(float timeDelta)
{
    Camera::Update(timeDelta);
}

void Camera_Cinematic::Late_Update(float timeDelta)
{
    Camera::Late_Update(timeDelta);
}

HRESULT Camera_Cinematic::Render()
{
    return Camera::Render();
}

void Camera_Cinematic::Apply_CinematicState(const Vec3& pos, const Quat& rot, float fovY)
{
    _transformCom->Set_LocalPosition(pos);
    _transformCom->Set_LocalRotation(rot);
    _fovY = fovY;

    Update_TransformMatrices();
}

Matrix Camera_Cinematic::Get_ViewMatrix() const
{
    Vec3 pos = _transformCom->Get_WorldPosition();
    Vec3 forward = _transformCom->Get_WorldForward();
    Vec3 up = _transformCom->Get_WorldUp();

    return XMMatrixLookAtLH(pos, pos + forward, up);
}

Matrix Camera_Cinematic::Get_ProjMatrix() const
{
    float width = max(1.f, GAME->Get_ViewportWidth());
    float height = max(1.f, GAME->Get_ViewportHeight());

    float aspect = width / height;

    return XMMatrixPerspectiveFovLH(
        _fovY,
        aspect,
        _nearZ,
        _farZ);
}

void Camera_Cinematic::Update_FreeMode(float timeDelta)
{
    // Camera_Free와 동일한 조작
    if (INPUT->KeyPress(KEY_TYPE::RBUTTON))
    {
        // WASD 이동
        if (INPUT->KeyPress(KEY_TYPE::W))
            _transformCom->Add_WorldOffset(_transformCom->Get_WorldForward() * _cameraSpeed * timeDelta);
        if (INPUT->KeyPress(KEY_TYPE::S))
            _transformCom->Add_WorldOffset(-_transformCom->Get_WorldForward() * _cameraSpeed * timeDelta);
        if (INPUT->KeyPress(KEY_TYPE::A))
            _transformCom->Add_WorldOffset(-_transformCom->Get_WorldRight() * _cameraSpeed * timeDelta);
        if (INPUT->KeyPress(KEY_TYPE::D))
            _transformCom->Add_WorldOffset(_transformCom->Get_WorldRight() * _cameraSpeed * timeDelta);

        // 마우스 회전
        Vec2 mouseDelta = INPUT->GetMouseDelta();
        if (mouseDelta.x != 0.f)
            _transformCom->Rotate_Axis(Vec3::Up, mouseDelta.x * _mouseSensor * timeDelta);
        if (mouseDelta.y != 0.f)
            _transformCom->Rotate_Axis(_transformCom->Get_WorldRight(), mouseDelta.y * _mouseSensor * timeDelta);
    }

    
    float wheel = INPUT->GetMouseWheel();
    if (wheel != 0.f)
        _cameraSpeed = ::clamp(_cameraSpeed + wheel * 2.f, 1.f, 100.f);

}

void Camera_Cinematic::Update_TargetMode(float timeDelta)
{
    // Camera_Target과 유사
    auto target = _targetTransform.lock();
    if (!target) return;

    // 마우스 드래그로 pitch/yaw 조정
    Vec2 mouseDelta = INPUT->GetMouseDelta();
    if (INPUT->KeyPress(KEY_TYPE::RBUTTON) || INPUT->KeyPress(KEY_TYPE::LBUTTON))
    {
        _yaw += mouseDelta.x * _mouseSensor;
        _pitch += mouseDelta.y * _mouseSensor;
        _pitch = ::clamp(_pitch, -80.f, 80.f);
    }

    
    float wheel = INPUT->GetMouseWheel();
    if (wheel != 0.f)
        _distance = ::clamp(_distance - wheel * 2.f, 1.f, 50.f);

    
    float pitchRad = XMConvertToRadians(_pitch);
    float yawRad = XMConvertToRadians(_yaw);

    Vec3 targetPos = target->Get_WorldPosition() + _targetOffset;
    Vec3 camOffset;

    camOffset.x = -cosf(pitchRad) * sinf(yawRad) * _distance;
    camOffset.y = sinf(pitchRad) * _distance;
    camOffset.z = -cosf(pitchRad) * cosf(yawRad) * _distance;

    _transformCom->Set_LocalPosition(targetPos + camOffset);
    _transformCom->LookAt(targetPos);
}

void Camera_Cinematic::Update_LookAtMode(float timeDelta)
{
    auto target = _targetTransform.lock();
    if (!target) return;

    Vec3 targetPos = target->Get_WorldPosition() + _targetOffset;
    _transformCom->LookAt(targetPos);
}

Shared<Camera_Cinematic> Camera_Cinematic::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Camera_Cinematic>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Camera_Cinematic");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Camera_Cinematic::Clone(void* arg)
{
    auto clone = make_shared<Camera_Cinematic>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Camera_Cinematic");

        return nullptr;
    }

    return clone;
}

void Camera_Cinematic::Free()
{
    Camera::Free();
}
