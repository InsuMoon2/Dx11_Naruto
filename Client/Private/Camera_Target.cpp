#include "pch.h"
#include "Camera_Target.h"

#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(Camera_Target, Protocol::OBJECT_TYPE_CAMERA_TARGET)
IMPLEMENT_REFLECTION(Camera_Target);

bool Camera_Target::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "Camera_Target";

    PROPERTY_VEC3("Offset : ", _offset, 0.1f);
    PROPERTY_FLOAT("Mouse Sensor", _mouseSensor, 0.1f, 20.f);  
    PROPERTY_FLOAT("Distance", _distance, 1.f, 50.f);  
    PROPERTY_FLOAT("Follow Speed", _followSpeed, 0.f, 20.f);   

    return true;
}

Camera_Target::Camera_Target(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Camera(device, context)
{
    
}

Camera_Target::Camera_Target(const Camera_Target& rhs)
    : Camera(rhs)
    , _offset(rhs._offset)
    , _followSpeed(rhs._followSpeed)
{
}

HRESULT Camera_Target::Initialize_Prototype()
{
    Camera::Initialize_Prototype();

    return S_OK;
}

HRESULT Camera_Target::Initialize(void* arg)
{
    if (arg == nullptr)
        return E_FAIL;

    FCameraTargetDesc* desc = static_cast<FCameraTargetDesc*>(arg);
    _offset = desc->offset;
    _followSpeed = desc->followSpeed;
    _enableMouseRotation = desc->enableMouseRotation;
    _bindOnPlayerSpawned = desc->bindOnPlayerSpawned;

    _distance = 3.f;

    LOG_INFO("Camera_Target: before Camera::Initialize");
    CHECK_FAILED(Camera::Initialize(arg), E_FAIL);
    LOG_INFO("Camera_Target: Initialize OK");

    return S_OK;
}

void Camera_Target::BeginPlay()
{
    Camera::BeginPlay();

    GAME->Set_ActiveCamera(GetSharedPtr<Camera>());

    if (_bindOnPlayerSpawned)
    {
        GAME->Get_DelegateHub().OnPlayerSpawned.Add(
            this, &Camera_Target::Set_TargetTransform);
    }
}

void Camera_Target::Priority_Update(float timeDelta)
{
    Camera::Priority_Update(timeDelta);

    if (!GAME->IsPlaying())
        return;

    auto target = _targetTransform.lock();
    if (!target) return;

    if (_enableMouseRotation)
    {
        float dx = INPUT->GetMouseDelta().x;
        float dy = INPUT->GetMouseDelta().y;

        _yaw += dx * _mouseSensor;
        _pitch += dy * _mouseSensor;

        _pitch = ::clamp(_pitch, _pitchMin, _pitchMax);

        // 줌
        float wheel = INPUT->GetMouseWheel();
        _distance -= wheel * _zoomSpeed;
        _distance = ::clamp(_distance, _distanceMin, _distanceMax);
    }

    float pitchRad = XMConvertToRadians(_pitch);
    float yawRad = XMConvertToRadians(_yaw);

    Vec3 targetPos = target->Get_WorldPosition();
    targetPos.y += _heightOffset;  
    Vec3 camOffset;

    camOffset.x = -cosf(pitchRad) * sinf(yawRad) * _distance;
    camOffset.y = sinf(pitchRad) * _distance;
    camOffset.z = -cosf(pitchRad) * cosf(yawRad) * _distance;

    Vec3 desiredPos = targetPos + camOffset;

    // 위치 보간
    Vec3 currentPos = _transformCom->Get_LocalPosition();
    Vec3 newPos = Vec3::Lerp(currentPos, desiredPos, _followSpeed * timeDelta);

    //_transformCom->Set_LocalPosition(newPos);
    _transformCom->Set_LocalPosition(desiredPos);
    _transformCom->LookAt(targetPos);

    Update_TransformMatrices();
}

void Camera_Target::Update(float timeDelta)
{
    Camera::Update(timeDelta);
}

void Camera_Target::Late_Update(float timeDelta)
{
    Camera::Late_Update(timeDelta);
}

HRESULT Camera_Target::Render()
{
    Camera::Render();


    return S_OK;
}

Shared<Camera_Target> Camera_Target::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Camera_Target>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Camera_Target");
        instance->Free();

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Camera_Target::Clone(void* arg)
{
    auto clone = make_shared<Camera_Target>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Camera_Target");
        clone->Free();

        return nullptr;
    }

    return clone;
}

void Camera_Target::Free()
{
    Camera::Free();
}
