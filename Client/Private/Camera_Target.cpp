#include "pch.h"
#include "Camera_Target.h"

#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(Camera_Target, Protocol::OBJECT_TYPE_CAMERA_TARGET)
IMPLEMENT_REFLECTION(Camera_Target);

bool Camera_Target::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "Camera_Target";

    PROPERTY_FLOAT("Distance", _distance, 1.f, 50.f);
    PROPERTY_FLOAT("Target Distance", _targetDistance, 1.f, 50.f);

    PROPERTY_FLOAT("Follow Speed", _followSpeed, 0.f, 20.f);
    PROPERTY_FLOAT("Zoom Lerp Speed", _zoomLerpSpeed, 0.f, 30.f);

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
    , _pitch(rhs._pitch)
    , _yaw(rhs._yaw)
    , _heightOffset(rhs._heightOffset)
    , _mouseSensor(rhs._mouseSensor)
    , _pitchMin(rhs._pitchMin)
    , _pitchMax(rhs._pitchMax)
    , _distance(rhs._distance)
    , _targetDistance(rhs._targetDistance)
    , _distanceMin(rhs._distanceMin)
    , _distanceMax(rhs._distanceMax)
    , _zoomSpeed(rhs._zoomSpeed)
    , _zoomLerpSpeed(rhs._zoomLerpSpeed)
    , _enableMouseRotation(rhs._enableMouseRotation)
    , _bindOnPlayerSpawned(rhs._bindOnPlayerSpawned)
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

    _targetDistance = _distance;

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
        _targetDistance -= wheel * _zoomSpeed;
        _targetDistance = ::clamp(_targetDistance, _distanceMin, _distanceMax);
    }

    float zoomAlpha = std::clamp(_zoomLerpSpeed * timeDelta, 0.f, 1.f);
    _distance = std::lerp(_distance, _targetDistance, zoomAlpha);
    _distance = ::clamp(_distance, _distanceMin, _distanceMax);

    float pitchRad = XMConvertToRadians(_pitch);
    float yawRad = XMConvertToRadians(_yaw);

    Vec3 targetPos = target->Get_WorldPosition();
    targetPos.y += _heightOffset;

    Vec3 camOffset;
    camOffset.x = -cosf(pitchRad) * sinf(yawRad) * _distance;
    camOffset.y = sinf(pitchRad) * _distance;
    camOffset.z = -cosf(pitchRad) * cosf(yawRad) * _distance;

    Vec3 desiredPos = targetPos + camOffset;

    // 보간처리
    Vec3  currentPos = _transformCom->Get_LocalPosition();
    float followAlpha = std::clamp(_followSpeed * timeDelta, 0.f, 1.f);
    Vec3  newPos = Vec3::Lerp(currentPos, desiredPos, followAlpha);

    _transformCom->Set_LocalPosition(newPos);
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
